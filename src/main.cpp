#pragma once
#include <cassert>

#include "clang-c/Index.h"

#include <filesystem>
#include <format>
#include <fstream>

#include "database.h"
#include "clang-database.h"

#include "struct/enum.h"

#define ALIGN_UP(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

template < class... Ts >
struct overloads : Ts...
{
    using Ts::operator()...;
};


type_id insert_type( const CXType& type )
{
    auto lower_type = type;
    while ( true )
    {
        // if we already have seen this type that means all the base types have been created
        if ( clang_database_t::get( ).is_type_defined( lower_type ) )
            break;

        if ( lower_type.kind == CXType_ConstantArray )
        {
            array_t arr( true, clang_getArraySize( lower_type ) );
            clang_database_t::get( ).save_type_id( lower_type, type_database_t::get( ).insert_type( arr ) );

            lower_type = clang_getArrayElementType( lower_type );
        }
        else if ( lower_type.kind == CXType_Pointer )
        {
            pointer_t ptr( clang_getArraySize( lower_type ) );
            clang_database_t::get( ).save_type_id( lower_type, type_database_t::get( ).insert_type( ptr ) );

            lower_type = clang_getPointeeType( lower_type );
        }
        else
        {
            // we reached the base type, verify that it exists
            assert( clang_database_t::get( ).is_type_defined( lower_type ), "type id must exist" );
            break;
        };
    }

    return clang_database_t::get( ).get_type_id( type );
}

struct client_data_t
{
    bool failed = false;
};

CXChildVisitResult visit_cursor( CXCursor cursor, CXCursor parent, CXClientData data )
{
    client_data_t* client_data = static_cast<client_data_t*>(data);

    const auto cursor_kind = clang_getCursorKind( cursor );
    switch ( cursor_kind )
    {
    case CXCursor_ClassDecl:
    case CXCursor_StructDecl:
    case CXCursor_UnionDecl:
        {
            // todo: in ctor check that none of the arguments are negative other than size = -1
            const auto cursor_type = clang_getCursorType( cursor );
            const auto type_name = clang_spelling_str( cursor );

            auto decl_type_byte_size = clang_Type_getSizeOf( cursor_type );
            assert(
                decl_type_byte_size != CXTypeLayoutError_Invalid &&
                decl_type_byte_size != CXTypeLayoutError_Incomplete &&
                decl_type_byte_size != CXTypeLayoutError_Dependent,
                "structure is not properly sized"
            );

            auto decl_type_byte_align = clang_Type_getAlignOf( cursor_type );
            assert( decl_type_byte_align > 0, "structure is not properly aligned" );

            uint32_t decl_type_size = decl_type_byte_size * 8;
            uint32_t decl_type_align = decl_type_byte_align * 8;
            structure_t sm{
                structure_settings{
                    .align = decl_type_align,
                    .size = decl_type_size,
                    .is_union = cursor_kind == CXCursor_UnionDecl,
                    .name = type_name
                },
                true
            };

            auto decl_type_id = type_database_t::get( ).insert_type( sm );
            clang_database_t::get( ).save_type_id( clang_getCursorType( cursor ), decl_type_id );

            const auto parent_cursor = clang_getCursorSemanticParent( cursor );
            if ( cursor_kind == CXCursor_UnionDecl && !clang_Cursor_isNull( parent_cursor ) &&
                clang_Cursor_isAnonymous( cursor ) )
            {
                // check if anonymous union because it must mean
                auto parent_type = clang_getCursorType( parent_cursor );
                std::visit<type_id_data>(
                    overloads
                    {
                        [&] ( structure_t& s )
                        {
                            const auto fields = s.get_fields( );

                            uint32_t target_bit_offset = 0;
                            if ( !s.is_union && !fields.empty( ) )
                            {
                                auto& back_field = fields.back( );
                                const auto prev_end_offset = back_field.bit_offset + ( back_field.bit_size + 7 ) / 8;
                                const auto union_align = clang_Type_getAlignOf( parent_type );

                                target_bit_offset = ALIGN_UP( prev_end_offset, union_align );
                            }

                            s.add_field(
                                base_field_t{
                                    .bit_offset = target_bit_offset,
                                    .bit_size = decl_type_size,
                                    .is_bit_field = false,
                                    .type_id = decl_type_id,
                                    .name = ""
                                } );
                        },
                        [] ( auto&& )
                        {
                            assert( true, "unexpected exception occurred" );
                        }
                    }, type_database_t::get( ).lookup_type( clang_database_t::get( ).get_type_id( parent_type ) ) );
            }

            return cursor_kind == CXCursor_ClassDecl ? CXChildVisit_Continue : CXChildVisit_Recurse;
        }
    case CXCursor_EnumDecl:
        {
            enum_t em{ clang_database_t::get( ).get_create_type_id( clang_getEnumDeclIntegerType( cursor ) ) };
            clang_database_t::get( ).save_type_id(
                clang_getCursorType( cursor ), type_database_t::get( ).insert_type( em ) );
            break;
        }
    }

    switch ( cursor_kind )
    {
    case CXCursor_EnumConstantDecl:
        {
            const CXCursorKind parent_kind = clang_getCursorKind( parent );
            assert( parent_kind == CXCursor_EnumDecl, "parent is not enum declaration" );

            const CXType parent_type = clang_getCursorType( parent );
            auto em = std::get<enum_t>(
                type_database_t::get( ).lookup_type(
                    clang_database_t::get( ).get_type_id( parent_type ) ) );

            const auto type_name = clang_spelling_str( cursor );
            em.insert_member( clang_getEnumConstantDeclValue( cursor ), type_name );
            break;
        }
    case CXCursor_FieldDecl:
        {
            const CXCursorKind parent_kind = clang_getCursorKind( parent );
            assert(
                parent_kind == CXCursor_ClassDecl || parent_kind == CXCursor_StructDecl,
                "parent is not valid structure declaration" );

            auto parent_type = clang_getCursorType( parent );
            auto underlying_type = clang_getCursorType( cursor );

            std::visit<type_id_data>(
                overloads
                {
                    [&] ( structure_t& s )
                    {
                        const auto bit_width = clang_getFieldDeclBitWidth( cursor );
                        assert( bit_width != -1, "bit width must not be value dependent" );

                        const auto bit_offset = clang_Cursor_getOffsetOfField( cursor );
                        assert(
                            bit_offset != CXTypeLayoutError_Invalid &&
                            bit_offset != CXTypeLayoutError_Incomplete &&
                            bit_offset != CXTypeLayoutError_Dependent &&
                            bit_offset != CXTypeLayoutError_InvalidFieldName,
                            "field offset is invalid" );

                        const type_id field_type_id = insert_type( underlying_type );

                        bool revised_field = false;

                        auto fields = s.get_fields( );
                        if ( !fields.empty( ) )
                        {
                            auto& back = fields.back( );
                            if ( back.bit_offset == bit_offset && back.type_id == field_type_id )
                            {
                                back.name = clang_spelling_str( cursor );
                                revised_field = true;
                            }
                        }

                        if ( !revised_field )
                        {
                            s.add_field(
                                base_field_t{
                                    .type_id = field_type_id,
                                    .bit_size = static_cast<uint32_t>(bit_width),
                                    .bit_offset = static_cast<uint32_t>(bit_offset),
                                    .is_bit_field = clang_Cursor_isBitField( cursor ) != 0
                                } );
                        }
                    },
                }, type_database_t::get( ).lookup_type( clang_database_t::get( ).get_type_id( parent_type ) ) );
            break;
        }
    case CXCursor_TypedefDecl:
        {
            CXType underlying_type = clang_getTypedefDeclUnderlyingType( cursor );
            if ( clang_database_t::get( ).is_type_defined( underlying_type ) ) // skip repeating typedefs
                return CXChildVisit_Recurse;

            switch ( underlying_type.kind )
            {
            case CXType_FunctionProto:
                {
                    call_conv conv = call_conv::unk;
                    switch ( clang_getFunctionTypeCallingConv( underlying_type ) )
                    {
                    case CXCallingConv_C: conv = call_conv::cdecl;
                        break;
                    case CXCallingConv_X86StdCall: conv = call_conv::stdcall;
                        break;
                    case CXCallingConv_X86FastCall: conv = call_conv::fastcall;
                        break;
                    case CXCallingConv_X86ThisCall: conv = call_conv::thiscall;
                        break;
                    }

                    function_t fun_proto{
                        clang_spelling_str( cursor ),
                        conv,
                        clang_database_t::get( ).get_type_id( clang_getResultType( underlying_type ) ),
                        [&] -> std::vector<type_id>
                        {
                            std::vector<type_id> types;
                            for ( auto i = 0; i < clang_getNumArgTypes( underlying_type ); i++ )
                                types.push_back(
                                    clang_database_t::get( ).get_type_id( clang_getArgType( underlying_type, i ) ) );

                            return types;
                        }( )
                    };

                    clang_database_t::get( ).save_type_id(
                        clang_getCursorType( cursor ), type_database_t::get( ).insert_type( fun_proto ) );
                    break;
                }
            default:
                {
                    const type_id id = insert_type( underlying_type );
                    if ( !clang_database_t::get( ).is_type_defined( underlying_type ) )
                    {
                        client_data->failed = true;
                        return CXChildVisit_Break;
                    }

                    const auto type_name = clang_spelling_str( cursor );
                    type_id typedef_id = type_database_t::get( ).insert_type(
                        alias_type_t(
                            type_name, id
                        ) );

                    clang_database_t::get( ).save_type_id( clang_getCursorType( cursor ), typedef_id );
                    break;
                }
            }
        }
    }

    return CXChildVisit_Recurse;
}

void parse_header( const std::string& header_data )
{
    const std::filesystem::path current_path = std::filesystem::current_path( );
    const std::filesystem::path stub_source = current_path / "stub_include.cpp";

    std::ofstream include_stub( stub_source, std::ios::out | std::ios::trunc );
    include_stub << header_data;

    include_stub.close( );

    std::vector<std::string> clang_args =
    {
        // "-I" + std::string(argv[1]),
        "-x",
        "c++",
        "-fms-extensions",
        "-Xclang",
        "-ast-dump",
        "-fsyntax-only",
        "-fms-extensions",
    };

    std::vector<const char*> c_args;
    for ( const auto& arg : clang_args )
        c_args.push_back( arg.c_str( ) );

    if ( const auto index = clang_createIndex( 0, 1 ) )
    {
        CXTranslationUnit tu = nullptr;
        const auto error = clang_parseTranslationUnit2(
            index,
            stub_source.string( ).c_str( ),
            c_args.data( ),
            static_cast<int>(c_args.size( )),
            nullptr,
            0,
            CXTranslationUnit_DetailedPreprocessingRecord |
            CXTranslationUnit_PrecompiledPreamble |
            CXTranslationUnit_SkipFunctionBodies |
            CXTranslationUnit_ForSerialization,
            &tu );

        if ( error == CXError_Success )
        {
            const CXCursor cursor = clang_getTranslationUnitCursor( tu );
            clang_visitChildren( cursor, visit_cursor, nullptr );
        }
        else
        {
            printf( "CXError: %d\n", error );
            return EXIT_FAILURE;
        }

        clang_disposeTranslationUnit( tu );
    }
}

int main( )
{
    const std::filesystem::path current_path = std::filesystem::current_path( );
    const std::filesystem::path stub_source = current_path / "stub_include.cpp";
    const std::vector<std::string> target_headers = {
        "phnt.h"
    };

    std::ofstream include_stub( stub_source, std::ios::out | std::ios::trunc );
    for ( auto& include : target_headers )
        include_stub << std::format( "#include <{}>\n", include );

    include_stub.close( );
}
