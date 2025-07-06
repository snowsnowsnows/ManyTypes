#include "manytypes-lib/manytypes.h"
#include "manytypes-lib/exceptions.h"

#include "manytypes-lib/formatter/clang.h"
#include "manytypes-lib/formatter/x64dbg.h"
#include "manytypes-lib/util/util.h"

#include <functional>
#include <iostream>
#include <ranges>

#define ALIGN_UP( value, alignment ) ( ( ( value ) + ( alignment ) - 1 ) & ~( ( alignment ) - 1 ) )

namespace mt
{
type_id database_update_insert( clang_context_t* client_data, const CXType& type, const type_id_data& data )
{
    type_id out;
    if ( client_data->clang_db.is_type_defined( type ) )
    {
        out = client_data->clang_db.get_type_id( type );
        client_data->type_db.update_type( out, data );
    }
    else
    {
        out = client_data->type_db.insert_type( data );
        client_data->clang_db.save_type_id( type, out );
    }

    return out;
}

// https://web.archive.org/web/20210511125654/https://joshpeterson.github.io/identifying-a-forward-declaration-with-libclang
bool is_forward_declaration( const CXCursor& cursor )
{
    const auto definition = clang_getCursorDefinition( cursor );
    if ( clang_equalCursors( definition, clang_getNullCursor() ) )
        return true;

    return !clang_equalCursors( cursor, definition );
}

std::string debug_print_cursor( const CXCursor& cursor )
{
    const CXSourceLocation location = clang_getCursorLocation( cursor );

    CXFile file;
    unsigned line, column;
    clang_getSpellingLocation( location, &file, &line, &column, nullptr );

    if ( file )
    {
        CXString file_name_str = clang_getFileName( file );
        std::string filename = clang_getCString( file_name_str );

        clang_disposeString( file_name_str );
        return std::format( "{}:{}:{}", filename, line, column );
    }

    return "";
}

type_id unwind_complex_type( clang_context_t* client_data, const CXType& type )
{
    auto lower_type = type;

    bool base_type_case = false;
    while ( !base_type_case )
    {
        // if we already have seen this type that means all the base types have been created
        if ( client_data->clang_db.is_type_defined( lower_type ) )
        {
            const auto lower_type_id = client_data->clang_db.get_type_id( lower_type );
            if ( !std::holds_alternative<null_type_t>( client_data->type_db.lookup_type( lower_type_id ) ) )
                break;
        }

        auto is_const = clang_isConstQualifiedType( lower_type );
        auto is_volatile = clang_isVolatileQualifiedType( lower_type );
        auto is_restrict = clang_isRestrictQualifiedType( lower_type );

        if ( is_const || is_volatile || is_restrict )
        {
            type_id pointee_type_id;

            auto pointee_type = clang_getUnqualifiedType( lower_type );
            if ( !client_data->clang_db.is_type_defined( pointee_type ) )
            {
                pointee_type_id = client_data->type_db.insert_placeholder_type( null_type_t() );
                client_data->clang_db.save_type_id( pointee_type, pointee_type_id );
            }
            else
                pointee_type_id = client_data->clang_db.get_type_id( pointee_type );

            qualified_t qualified( pointee_type_id, is_const, is_volatile, is_restrict );
            database_update_insert( client_data, lower_type, qualified );

            lower_type = pointee_type;
            continue;
        }

        switch ( lower_type.kind )
        {
        case CXType_IncompleteArray:
        case CXType_ConstantArray:
        {
            type_id pointee_type_id;

            auto element_type = clang_getElementType( lower_type );
            if ( !client_data->clang_db.is_type_defined( element_type ) )
            {
                pointee_type_id = client_data->type_db.insert_placeholder_type( null_type_t() );
                client_data->clang_db.save_type_id( element_type, pointee_type_id );
            }
            else
                pointee_type_id = client_data->clang_db.get_type_id( element_type );

            if ( lower_type.kind == CXType_IncompleteArray )
            {
                array_t arr( pointee_type_id, clang_Type_getSizeOf( element_type ) * 8 );
                database_update_insert( client_data, lower_type, arr );
            }
            else
            {
                array_t arr( pointee_type_id, clang_getNumElements( lower_type ), clang_Type_getSizeOf( element_type ) * 8 );
                database_update_insert( client_data, lower_type, arr );
            }

            lower_type = element_type;
            break;
        }
        case CXType_Pointer:
        case CXType_LValueReference:
        {
            type_id pointee_type_id;

            auto pointee_type = clang_getPointeeType( lower_type );
            if ( !client_data->clang_db.is_type_defined( pointee_type ) )
            {
                pointee_type_id = client_data->type_db.insert_placeholder_type( null_type_t() );
                client_data->clang_db.save_type_id( pointee_type, pointee_type_id );
            }
            else
                pointee_type_id = client_data->clang_db.get_type_id( pointee_type );

            uint32_t target_size;
            if ( lower_type.kind == CXType_Pointer )
                target_size = clang_Type_getSizeOf( lower_type ) * 8;
            else
                target_size = client_data->type_db.bit_pointer_size;

            pointer_t ptr( pointee_type_id, target_size );
            database_update_insert( client_data, lower_type, ptr );

            lower_type = pointee_type;
            break;
        }
        case CXType_FunctionProto:
        {
            call_conv conv = call_conv::unk;
            switch ( clang_getFunctionTypeCallingConv( lower_type ) )
            {
            case CXCallingConv_C:
                conv = call_conv::cc_cdecl;
                break;
            case CXCallingConv_X86StdCall:
                conv = call_conv::cc_stdcall;
                break;
            case CXCallingConv_X86FastCall:
                conv = call_conv::cc_fastcall;
                break;
            case CXCallingConv_X86ThisCall:
                conv = call_conv::cc_thiscall;
                break;
            }

            function_t fun_proto{
                conv,
                unwind_complex_type( client_data, clang_getResultType( lower_type ) ),
                [&]() -> std::vector<type_id>
                {
                    std::vector<type_id> types;
                    for ( auto i = 0; i < clang_getNumArgTypes( lower_type ); i++ )
                    {
                        auto arg_type = clang_getArgType( lower_type, i );
                        auto unwind = unwind_complex_type( client_data, arg_type );

                        types.push_back( unwind );
                    }

                    return types;
                }()
            };

            database_update_insert( client_data, lower_type, fun_proto );
            break;
        }
        case CXType_Elaborated:
        {
            type_id named_type_id;

            const auto named_type = clang_Type_getNamedType( lower_type );
            if ( !client_data->clang_db.is_type_defined( named_type ) )
            {
                named_type_id = client_data->type_db.insert_placeholder_type( null_type_t() );
                client_data->clang_db.save_type_id( clang_Type_getNamedType( lower_type ), named_type_id );
            }
            else
                named_type_id = client_data->clang_db.get_type_id( named_type );

            // todo, this seems like a terrible idea
            auto opt_data = get_elaborated_string_data( lower_type );
            if ( !opt_data )
            {
                elaborated_t alias( named_type_id );
                database_update_insert( client_data, lower_type, alias );
            }
            else
            {
                auto& [prefix, scope, type_name] = *opt_data;
                elaborated_t alias( named_type_id, prefix, scope, type_name );
                database_update_insert( client_data, lower_type, alias );
            }

            lower_type = named_type;
            break;
        }
        default:
        {
            // we reached the base type, verify that it exists
            auto test = clang_spelling_str( lower_type );
            if ( !client_data->clang_db.is_type_defined( lower_type ) )
                throw TypeNotDefinedException( "type id must exist" );
            base_type_case = true;

            break;
        }
        }
    }

    return client_data->clang_db.get_type_id( type );
}

CXChildVisitResult visit_cursor( CXCursor cursor, CXCursor parent, CXClientData data )
{
    clang_context_t* client_data = static_cast<clang_context_t*>( data );

    // dont know dont care
    if ( !clang_Cursor_isNull( clang_getSpecializedCursorTemplate( cursor ) ) )
        return CXChildVisit_Continue;

    const auto cursor_kind = clang_getCursorKind( cursor );
    switch ( cursor_kind )
    {
    case CXCursor_ClassDecl:
    case CXCursor_StructDecl:
    case CXCursor_UnionDecl:
    {
        const auto cursor_type = clang_getCursorType( cursor );

        // todo: in ctor check that none of the arguments are negative other than size = -1
        auto decl_type_byte_size = clang_Type_getSizeOf( cursor_type );
        if ( decl_type_byte_size == CXTypeLayoutError_Invalid || decl_type_byte_size == CXTypeLayoutError_Dependent )
            throw InvalidStructureException( "structure is not properly sized" );

        bool is_forward = is_forward_declaration( cursor );

        auto decl_type_byte_align = clang_Type_getAlignOf( cursor_type );
        if ( !is_forward && decl_type_byte_align <= 0 )
            throw InvalidStructureException( "structure is not properly aligned" );

        uint32_t decl_type_size = decl_type_byte_size * 8;
        uint32_t decl_type_align = decl_type_byte_align * 8;

        std::string type_name = clang_spelling_str( cursor );
        structure_t sm{
            structure_settings{
                .name = type_name,
                .align = decl_type_align,
                .size = decl_type_size,
                .is_union = cursor_kind == CXCursor_UnionDecl,
                .is_forward = is_forward },
            true
        };

        type_id decl_type_id;
        if ( client_data->clang_db.is_type_defined( cursor_type ) )
        {
            // type already has been declared,
            decl_type_id = client_data->clang_db.get_type_id( cursor_type );

            auto type_info = client_data->type_db.lookup_type( decl_type_id );
            if ( !std::holds_alternative<null_type_t>( type_info ) )
            {
                // todo revist this, im not sure this is the way of going about this
                auto& prev_decl = std::get<structure_t>( type_info );

                // todo maybe throw exception if sizes mismatch and both are not forward
                auto settings = prev_decl.get_settings();
                if ( !settings.is_forward || is_forward ) // if prev decl is forward and this one is not, we want to replace
                    return CXChildVisit_Continue;         // but this is not so we can continue
            }

            client_data->type_db.update_type( decl_type_id, sm );
        }
        else
        {
            decl_type_id = client_data->type_db.insert_type( sm );
            client_data->clang_db.save_type_id( cursor_type, decl_type_id );
        }

        const auto parent_cursor = clang_getCursorSemanticParent( cursor );
        if ( ( clang_Cursor_isAnonymousRecordDecl( cursor ) || clang_Cursor_isAnonymous( cursor ) ) && !clang_Cursor_isNull( parent_cursor ) )
        {
            // check if anonymous union because it must mean
            auto parent_type = clang_getCursorType( parent_cursor );
            std::visit(
                overloads{
                    [&]( structure_t& s )
                    {
                        const auto fields = s.get_fields();

                        uint32_t target_bit_offset = 0;
                        if ( !s.is_union() && !fields.empty() )
                        {
                            auto& back_field = fields.back();
                            const auto prev_end_offset = ( back_field.bit_offset + ( back_field.bit_size + 7 ) ) / 8;
                            const auto union_align = clang_Type_getAlignOf( cursor_type );

                            target_bit_offset = ALIGN_UP( prev_end_offset, union_align ) * 8;
                        }

                        s.add_field(
                            base_field_t{
                                .bit_offset = target_bit_offset,
                                .bit_size = decl_type_size,
                                .is_bit_field = false,
                                .name = "",
                                .type_id = decl_type_id,
                            } );
                    },
                    [&]( auto&& )
                    {
                        throw InvalidParentDeclarationException( "unexpected parent for anonymous type", debug_print_cursor( cursor ) );
                    } },
                client_data->type_db.lookup_type( client_data->clang_db.get_type_id( parent_type ) ) );
        }

        return cursor_kind == CXCursor_ClassDecl ? CXChildVisit_Continue : CXChildVisit_Recurse;
    }
    case CXCursor_EnumDecl:
    {
        auto cursor_type = clang_getCursorType( cursor );

        const auto type_name = clang_spelling_str( cursor );
        bool is_forward = is_forward_declaration( cursor );

        enum_t em{
            enum_settings{
                .name = type_name,
                .underlying = client_data->clang_db.get_type_id( clang_getEnumDeclIntegerType( cursor ) ),
                .size = static_cast<uint32_t>( clang_Type_getSizeOf( cursor_type ) ) * 8,
                .is_forward = is_forward,
            }
        };

        type_id decl_type_id;
        if ( client_data->clang_db.is_type_defined( cursor_type ) )
        {
            decl_type_id = client_data->clang_db.get_type_id( cursor_type );

            auto type_info = client_data->type_db.lookup_type( decl_type_id );
            if ( !std::holds_alternative<null_type_t>( type_info ) )
            {
                auto& prev_decl = std::get<enum_t>( type_info );
                if ( !prev_decl.get_settings().is_forward || is_forward )
                    return CXChildVisit_Continue;
            }

            client_data->type_db.update_type( decl_type_id, em );
        }
        else
        {
            decl_type_id = client_data->type_db.insert_type( em );
            client_data->clang_db.save_type_id( cursor_type, decl_type_id );
        }

        break;
    }
    }

    switch ( cursor_kind )
    {
    case CXCursor_EnumConstantDecl:
    {
        const CXCursorKind parent_kind = clang_getCursorKind( parent );
        if ( parent_kind != CXCursor_EnumDecl )
            throw InvalidParentDeclarationException( "parent is not enum declaration", debug_print_cursor( cursor ) );

        const CXType parent_type = clang_getCursorType( parent );
        enum_t& em = std::get<enum_t>(
            client_data->type_db.lookup_type(
                client_data->clang_db.get_type_id( parent_type ) ) );

        const auto type_name = clang_spelling_str( cursor );
        em.insert_member( clang_getEnumConstantDeclValue( cursor ), type_name );
        break;
    }
    case CXCursor_FieldDecl:
    {
        const CXCursorKind parent_kind = clang_getCursorKind( parent );
        if ( parent_kind != CXCursor_ClassDecl && parent_kind != CXCursor_StructDecl && parent_kind != CXCursor_UnionDecl )
            throw InvalidParentDeclarationException( "parent is not valid structure declaration", debug_print_cursor( cursor ) );

        auto parent_type = clang_getCursorType( parent );
        auto underlying_type = clang_getCursorType( cursor );

        type_id_data& type_data = client_data->type_db.lookup_type( client_data->clang_db.get_type_id( parent_type ) );
        std::visit(
            overloads{
                [&]( structure_t& s )
                {
                    long long bit_width;
                    if ( !clang_Cursor_isBitField( cursor ) )
                    {
                        const auto field_size = clang_Type_getSizeOf( underlying_type );
                        bit_width = field_size * 8;

                        if ( field_size == CXTypeLayoutError_Incomplete )
                        {
                            if ( underlying_type.kind != CXType_IncompleteArray && ( underlying_type.kind != CXType_ConstantArray || clang_getArraySize( underlying_type ) != 0 ) )
                                throw InvalidFieldException( "incomplete layout must be of incomplete array type", debug_print_cursor( cursor ) );

                            bit_width = 0;
                        }
                        else if ( field_size < 0 )
                            throw InvalidFieldException( "bit width must not be value dependent", debug_print_cursor( cursor ) );
                    }
                    else
                    {
                        bit_width = clang_getFieldDeclBitWidth( cursor );
                        if ( bit_width < 0 )
                            throw InvalidFieldException( "bit width must not be value dependent", debug_print_cursor( cursor ) );
                    }

                    const auto bit_offset = clang_Cursor_getOffsetOfField( cursor );
                    if ( bit_offset == CXTypeLayoutError_Invalid || bit_offset == CXTypeLayoutError_Incomplete || bit_offset == CXTypeLayoutError_Dependent || bit_offset == CXTypeLayoutError_InvalidFieldName )
                        throw InvalidFieldException( "field offset is invalid", debug_print_cursor( cursor ) );

                    const type_id field_type_id = unwind_complex_type( client_data, underlying_type );
                    bool revised_field = false;

                    // this is a weird solution
                    // its intended to allow anonymous unions to exist and this will rename them if they are actually
                    // associated with a explicit field. i dont think theres any other way to tell
                    auto& fields = s.get_fields();
                    if ( !fields.empty() )
                    {
                        type_id true_underlying = field_type_id;
                        while ( true )
                        {
                            type_id_data& data = client_data->type_db.lookup_type( true_underlying );
                            if ( !std::holds_alternative<elaborated_t>( data ) && !std::holds_alternative<array_t>( data ) )
                                break;

                            if ( std::holds_alternative<elaborated_t>( data ) )
                            {
                                auto& elab = std::get<elaborated_t>( data );
                                true_underlying = elab.type;
                            }
                            else
                            {
                                auto& array = std::get<array_t>( data );
                                true_underlying = array.get_elem_type();
                            }
                        }

                        auto& back = fields.back();
                        if ( back.bit_offset == bit_offset && back.name.empty() &&
                             back.type_id == true_underlying )
                        {
                            // anonymous field
                            back.name = clang_spelling_str( cursor );
                            back.type_id = field_type_id;

                            revised_field = true;
                        }
                    }

                    if ( !revised_field )
                    {
                        s.add_field(
                            base_field_t{
                                .bit_offset = static_cast<uint32_t>( bit_offset ),
                                .bit_size = static_cast<uint32_t>( bit_width ),
                                .is_bit_field = clang_Cursor_isBitField( cursor ) != 0,
                                .name = clang_spelling_str( cursor ),
                                .type_id = field_type_id,
                            } );
                    }
                },
                [&]( auto&& ) {},
            },
            type_data );
        break;
    }
    case CXCursor_TypedefDecl:
    {
        auto cursor_type = clang_getCursorType( cursor );
        if ( client_data->clang_db.is_type_defined( cursor_type ) ) // skip repeating typedefs
            return CXChildVisit_Recurse;

        // todo this is a hack because the way nullptr_t is declared leads to CXType_Unexposed
        // there may be a better and more formal way to handle this, but not now
        auto type_name = clang_spelling_str( cursor );
        if ( type_name.get() == "nullptr_t" )
            return CXChildVisit_Recurse;

        // debug_print_cursor( cursor );
        CXType underlying_type = clang_getTypedefDeclUnderlyingType( cursor );

        type_id result_id = unwind_complex_type( client_data, underlying_type );
        if ( !client_data->clang_db.is_type_defined( underlying_type ) )
            throw TypeNotDefinedException( "underlying type must be defined" );

        result_id = client_data->type_db.insert_type(
            typedef_type_t(
                type_name,
                result_id ) );

        client_data->clang_db.save_type_id( cursor_type, result_id );
        break;

        auto semantic_parent = clang_getCursorSemanticParent( cursor );
        switch ( clang_getCursorKind( semantic_parent ) )
        {
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
        case CXCursor_ClassDecl:
            client_data->type_db.insert_semantic_parent(
                result_id,
                client_data->clang_db.get_type_id( clang_getCursorType( semantic_parent ) ) );
            break;
        case CXCursor_TranslationUnit:
            break;
        default:
            throw InvalidParentDeclarationException( "typedef must be contained within a valid structure or class", debug_print_cursor( cursor ) );
        }
    }
    case CXCursor_CXXBaseSpecifier:
    {
        const CXCursorKind parent_kind = clang_getCursorKind( parent );
        if ( parent_kind != CXCursor_ClassDecl && parent_kind != CXCursor_StructDecl && parent_kind != CXCursor_UnionDecl )
            throw InvalidParentDeclarationException( "parent is not valid structure declaration", debug_print_cursor( cursor ) );

        auto parent_type = clang_getCursorType( parent );

        type_id_data& type_data = client_data->type_db.lookup_type( client_data->clang_db.get_type_id( parent_type ) );
        std::visit(
            overloads{
                [&]( structure_t& s )
                {
                    const auto bit_offset = clang_getOffsetOfBase( parent, cursor );
                    if ( bit_offset == CXTypeLayoutError_Invalid || bit_offset == CXTypeLayoutError_Incomplete || bit_offset == CXTypeLayoutError_Dependent || bit_offset == CXTypeLayoutError_InvalidFieldName )
                        throw InvalidFieldException( "field offset is invalid", debug_print_cursor( cursor ) );

                    auto underlying_type = clang_getCursorType( cursor );
                    auto bit_width = clang_Type_getSizeOf( underlying_type );
                    if ( bit_width < 0 )
                        throw InvalidFieldException( "bit width must not be value dependent", debug_print_cursor( cursor ) );

                    const type_id field_type_id = unwind_complex_type( client_data, underlying_type );
                    s.add_field(
                        base_field_t{
                            .bit_offset = static_cast<uint32_t>( bit_offset ),
                            .bit_size = static_cast<uint32_t>( bit_width ),
                            .is_bit_field = clang_Cursor_isBitField( cursor ) != 0,
                            .name = clang_spelling_str( cursor ),
                            .type_id = field_type_id,
                        } );
                },
                [&]( auto&& ) {},
            },
            type_data );

        break;
    }
    case CXCursor_ClassTemplate:
    case CXCursor_FunctionTemplate:
    case CXCursor_TemplateTypeParameter:
    case CXCursor_ClassTemplatePartialSpecialization:
    {
        // todo maybe add logging
        return CXChildVisit_Continue;
    }
    }

    return CXChildVisit_Recurse;
}

std::optional<type_database_t> parse_root_source( const std::filesystem::path& src_path, const bool bit32 )
{
    const std::vector<std::string> clang_args = {
        "-x",
        "c++",
        "-fms-extensions",
        "-Xclang",
        "-fsyntax-only",
        "-target",
        bit32 ? "i686-pc-windows-msvc" : "x86_64-pc-windows-msvc"
    };

    std::vector<const char*> c_args;
    for ( const auto& arg : clang_args )
        c_args.push_back( arg.c_str() );

#ifdef _DEBUG
    if ( const auto index = clang_createIndex( 0, 1 ) )
#else
    if ( const auto index = clang_createIndex( 0, 0 ) )
#endif
    {
        CXTranslationUnit tu = nullptr;
        const auto error = clang_parseTranslationUnit2(
            index,
            src_path.string().c_str(),
            c_args.data(),
            static_cast<int>( c_args.size() ),
            nullptr,
            0,
            CXTranslationUnit_DetailedPreprocessingRecord |
                CXTranslationUnit_PrecompiledPreamble |
                CXTranslationUnit_SkipFunctionBodies |
                CXTranslationUnit_ForSerialization,
            &tu );

        if ( error == CXError_Success )
        {
            /*
            const unsigned num_diagnostics = clang_getNumDiagnostics( tu );

            bool error_found = false;
            for ( unsigned i = 0; i < num_diagnostics && !error_found; i++ )
            {
                const CXDiagnostic diagnostic = clang_getDiagnostic( tu, i );
                const CXDiagnosticSeverity severity = clang_getDiagnosticSeverity( diagnostic );
                if ( severity == CXDiagnostic_Error || severity == CXDiagnostic_Fatal )
                    error_found = true;
#ifdef _DEBUG
                CXString formatted_diag = clang_formatDiagnostic(diagnostic, clang_defaultDiagnosticDisplayOptions());
                std::cout << "Diagnostic " << clang_getCString(formatted_diag) << std::endl << std::flush;
#endif
                clang_disposeDiagnostic( diagnostic );
            }

            if ( error_found )
                return std::nullopt;
            */

            clang_context_t ctx( bit32 ? 4 : 8 );

            const CXCursor cursor = clang_getTranslationUnitCursor( tu );
            clang_visitChildren( cursor, visit_cursor, &ctx );

            if ( !ctx.failed )
                return ctx.type_db;
        }
        else
            printf( "CXError: %d\n", error );

        clang_disposeTranslationUnit( tu );
    }

    return std::nullopt;
}

std::string create_header( type_database_t& db )
{
    formatter_clang fmt( db );
    return fmt.print_database();
}

std::string create_x64dbg_database( type_database_t& db )
{
    x64dbg_formatter fmt( db );
    return fmt.generate_json().dump( 4 );
}
} // namespace mt