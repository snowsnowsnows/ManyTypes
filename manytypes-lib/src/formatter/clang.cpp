#include "manytypes-lib/formatter/clang.h"
#include "manytypes-lib/util/util.h"
#include "manytypes-lib/exceptions.h"

#include <functional>
#include <ranges>
#include <format>

namespace mt
{
std::string formatter_clang::print_database()
{
    std::string result;
    result += print_forwards();
    result += print_typedefs();
    result += print_enums();
    result += print_structs();

    return result;
}
std::string formatter_clang::print_forwards()
{
    std::string result;

    for ( auto [type_id, type_id_data] : type_db.get_types() )
    {
        std::visit(
            overloads{
                [&result]( structure_t& s )
                {
                    if ( !s.get_name().empty() )
                        result += std::format( "{} {};\n", s.is_union() ? "union" : "struct", s.get_name() );
                },
                [&result]( enum_t& e )
                {
                    if ( !e.get_name().empty() )
                        result += std::format( "enum {};\n", e.get_name() );
                },
                [&result]( auto&& ) {} },
            type_id_data );
    }

    return result;
}

std::string formatter_clang::print_typedefs()
{
    std::string result;
    std::unordered_set<type_id> printed_typedefs;
    std::unordered_set<type_id> printed_stack;

    std::function<void( type_id )> recurse_typedef;
    recurse_typedef = [&]( const type_id id )
    {
        if ( printed_typedefs.contains( id ) )
            return;

        if ( printed_stack.contains( id ) )
            __debugbreak();

        printed_stack.insert( id );

        const auto type_data = type_db.lookup_type( id );
        auto result_string = std::visit(
            [&]( auto&& a ) -> std::string
            {
                using T = std::decay_t<decltype( a )>;
                if constexpr ( !std::is_same_v<T, structure_t> &&
                               !std::is_same_v<T, enum_t> )
                {
                    auto deps = a.get_dependencies();
                    for ( auto dep : deps )
                        recurse_typedef( dep );
                }

                if constexpr ( std::is_same_v<T, typedef_type_t> )
                    return print_forward_alias( a ) + ";\n";

                return "";
            },
            type_data );

        result += result_string;

        printed_typedefs.insert( id );
        printed_stack.erase( id );
    };

    for ( const auto type_id : type_db.get_types() | std::views::keys )
        recurse_typedef( type_id );

    return result;
}

std::string formatter_clang::print_enums()
{
    std::string result;

    for ( const auto& type_data : type_db.get_types() | std::views::values )
        if ( std::holds_alternative<enum_t>( type_data ) )
            result += print_enum( std::get<enum_t>( type_data ) ) + ";\n";

    return result;
}

std::string formatter_clang::print_structs()
{
    std::string result;
    std::unordered_set<type_id> printed_structures;
    std::unordered_set<type_id> printed_stack;

    std::function<void( type_id )> recruse_struct;
    recruse_struct = [&]( const type_id id )
    {
        if ( printed_structures.contains( id ) )
            return;

        if ( printed_stack.contains( id ) )
            __debugbreak();

        printed_stack.insert( id );

        auto type_data = type_db.lookup_type( id );
        auto result_string = std::visit(
            [&]( auto&& a ) -> std::string
            {
                using T = std::decay_t<decltype( a )>;
                if ( std::is_same_v<T, pointer_t> )
                    return "";

                auto deps = a.get_dependencies();
                for ( auto dep : deps )
                    recruse_struct( dep );

                if constexpr ( std::is_same_v<T, structure_t> )
                    if ( !a.get_name().empty() )
                        return print_structure( a ) + ";\n";

                return "";
            },
            type_data );

        result += result_string;

        printed_structures.insert( id );
        printed_stack.erase( id );
    };

    for ( const auto type_id : type_db.get_types() | std::views::keys )
        recruse_struct( type_id );

    return result;
}

void formatter_clang::print_identifier( const type_id& type, std::string& identifier )
{
    auto type_data = type_db.lookup_type( type );
    std::visit(
        overloads{
            [&]( const qualified_t& q )
            {
                std::string type_print;
                print_identifier( q.underlying, type_print );

                identifier = type_print + identifier;

                if ( q.is_const )
                    identifier = "const " + identifier;

                if ( q.is_volatile )
                    identifier = "volatile " + identifier;

                if ( q.is_restrict )
                    identifier = "restrict " + identifier;
            },
            [&]( const elaborated_t& f )
            {
                // add some sort of check to assert that this should be a basic type
                std::string type_print;
                print_identifier( f.type, type_print );

                identifier = type_print + identifier;
            },

            // base
            [&]( const typedef_type_t& a )
            {
                identifier = a.alias + " " + identifier;
            },
            [&]( const basic_type_t& b )
            {
                identifier = b.name + " " + identifier;
            },
            [&]( const structure_t& s )
            {
                if ( s.get_name().empty() )
                    identifier = print_type( type ) + " " + identifier;
                else
                    identifier = s.get_name() + " " + identifier;
            },
            [&]( const enum_t& e )
            {
                if ( e.get_name().empty() )
                    identifier = print_type( type ) + " " + identifier;
                else
                    identifier = e.get_name() + " " + identifier;
            },

            // complex types
            [&]( const function_t& f )
            {
                identifier += "(";

                const auto& args = f.get_args();
                if ( !args.empty() )
                {
                    for ( const auto& arg : args )
                    {
                        std::string arg_format;
                        print_identifier( arg, arg_format );

                        identifier += arg_format + ",";
                    }

                    identifier.pop_back();
                }

                identifier += ")";
                print_identifier( f.get_return_type(), identifier );
            },
            [&]( const pointer_t& p )
            {
                const auto& pointee = type_db.lookup_type( p.get_elem_type() );

                std::string ptr_name;
                if ( const auto ptr_size = p.get_ptr_bit_size(); ptr_size != type_db.bit_pointer_size )
                {
                    // x64dbg wont be able to read the pointer anyways, so we can break the chain
                    // and write in a non pointer type

                    switch ( ptr_size )
                    {
                    case 32:
                        ptr_name = " __ptr32 ";
                        break;
                    case 64:
                        ptr_name = " __ptr64 ";
                        break;
                    default:
                        throw InvalidPointerSizeException( "invalid pointer size detected" );
                    }
                }

                if ( std::holds_alternative<function_t>( pointee ) ||
                     std::holds_alternative<array_t>( pointee ) )
                    identifier = "(*" + ptr_name + identifier + ")";
                else
                    identifier = "*" + ptr_name + identifier;

                print_identifier( p.get_elem_type(), identifier );
            },
            [&]( const array_t& arr )
            {
                if ( arr.is_fixed_length() )
                {
                    auto size = arr.get_array_length();
                    identifier += std::format( "[{}]", size );
                }
                else
                    identifier += std::format( "[]" );

                print_identifier( arr.get_elem_type(), identifier );
            },
            []( const auto& a )
            {
                throw InvalidTypeException( "invalid type for identifier printer found" );
            } },
        type_data );
}

std::string formatter_clang::print_type( const type_id id, bool ignore_anonymous )
{
    return std::visit(
        overloads{
            [&]( structure_t& s )
            {
                if ( ignore_anonymous && s.get_name().empty() )
                    return std::string();

                return print_structure( s );
            },
            [&]( const enum_t& e )
            {
                if ( ignore_anonymous && e.get_name().empty() )
                    return std::string();

                return print_enum( e );
            },
            [&]( const typedef_type_t& t )
            {
                return print_forward_alias( t );
            },
            [&]( const auto& a ) -> std::string
            {
                return "";
            } },
        type_db.lookup_type( id ) );
}

std::string formatter_clang::print_structure( structure_t& s )
{
    if ( s.get_settings().is_forward )
        return "";

    std::string out;
    out += std::format( "{} {} {{", s.is_union() ? "union" : "struct", s.get_name() );

    for ( const auto& field : s.get_fields() )
    {
        std::string identifier = field.name;
        print_identifier( field.type_id, identifier );

        if ( !field.is_bit_field )
            out += std::format( "\n\t{};", identifier );
        else
            out += std::format( "\n\t{} : {};", identifier, field.bit_size );
    }

    out += "\n}";

    return out;
}

std::string formatter_clang::print_enum( const enum_t& e )
{
    std::string out;
    out += std::format( "enum {} {{", e.get_name() );

    for ( const auto& [value, name] : e.get_members() )
        out += std::format( "\n\t{} = {},", name, value );

    out += "\n}";

    return out;
}

std::string formatter_clang::print_forward_alias( const typedef_type_t& a )
{
    std::string out;

    std::string identifier = a.alias;
    print_identifier( a.type, identifier );

    out += std::format( "typedef {}", identifier );

    return out;
}

} // namespace mt