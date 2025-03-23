#include "manytypes-lib/formatter/clang.h"
#include "manytypes-lib/util/util.h"

#include <functional>
#include <ranges>

std::string formatter_clang::print_database()
{
    std::unordered_set<type_id> visited;
    std::unordered_set<type_id> rec_stack;
    std::vector<type_id> sorted;

    std::function<void( type_id type )> dfs_type;
    dfs_type = [&]( const type_id id )
    {
        assert( !rec_stack.contains( id ), "current dependency stack should not contain id. ciruclar dep" );
        if ( visited.contains( id ) )
            return;

        visited.insert( id );
        rec_stack.insert( id );

        std::vector<type_id> deps;

        const auto& type_info = type_db.lookup_type( id );
        std::visit(
            overloads{
                [&]( structure_t& s )
                {
                    deps = s.get_dependencies();
                },
                [&]( enum_t& e )
                {
                    deps = e.get_dependencies();
                },
                [&]( typedef_type_t& a )
                {
                    deps = { a.type };
                },
                [&]( pointer_t& p )
                {
                    // pointer breaks any dependency chain, no types before this need
                    // to be printed
                },
                [&]( auto&& )
                {
                    // should be trivial type such as an array or something, so we can add and skip
                    visited.insert( id );
                } },
            type_info );

        for ( const auto& dep : deps )
            dfs_type( dep );

        rec_stack.erase( id );
        sorted.push_back( id );
    };

    for ( const auto& type_id : type_db.get_types() | std::views::keys )
    {
        if ( !visited.contains( type_id ) )
            dfs_type( type_id );
    }

    std::string out;
    for ( type_id id : sorted )
        out += print_type( id );

    return out;
}

std::string formatter_clang::print_structure( const structure_t& s )
{
    std::string out;
    out += std::format( "struct {} {{", s.get_name() );

    for ( const auto& field : s.get_fields() )
    {
        std::string identifier = field.name;
        print_identifier( field.type_id, identifier, false );

        if ( !field.is_bit_field )
            out += std::format( "\n\t{};", identifier );
        else
            out += std::format( "\n\t{} : {};", identifier, field.bit_size );
    }

    out += "\n};";

    return out;
}

std::string formatter_clang::print_enum( const enum_t& e )
{
    std::string out;
    out += std::format( "enum {} {{", e.get_name() );

    for ( const auto& [value, name] : e.get_members() )
        out += std::format( "\n\t{} = {},", name, value );

    out += "\n};";

    return out;
}

std::string formatter_clang::print_forward_alias( const typedef_type_t& a )
{
    std::string out;

    std::string identifier = a.alias;
    print_identifier( a.type, identifier, false );

    out += std::format( "typedef {};", identifier );

    return out;
}

std::string formatter_clang::print_type( const type_id id )
{
    return std::visit(
        overloads{
            [&]( const structure_t& s )
            {
                return print_structure( s );
            },
            [&]( const enum_t& e )
            {
                return print_enum( e );
            },
            [&]( const typedef_type_t& t )
            {
                return print_forward_alias( t );
            },
            [&]( const auto& a ) -> std::string
            {
                assert( false && "unknown type visited" );
                return "";
            } },
        type_db.lookup_type( id ) );
}

void formatter_clang::print_identifier( const type_id& type, std::string& identifier, bool trivial_type )
{
    auto type_data = type_db.lookup_type( type );
    if ( trivial_type )
    {
        assert( std::holds_alternative<typedef_type_t>( type_data ) ||
                    std::holds_alternative<basic_type_t>( type_data ) ||
                    std::holds_alternative<structure_t>( type_data ) ||
                    std::holds_alternative<enum_t>( type_data ),
            "type must be basic type" );
    }

    std::visit(
        overloads{
            [&]( const elaborated_t& f )
            {
                // add some sort of check to assert that this should be a basic type
                std::string type_print;
                print_identifier( f.type, type_print, true );

                identifier = f.elaborated_name + " " + type_print + identifier;
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
                std::string prefix;
                if ( !trivial_type )
                {
                    if ( s.is_union() )
                        prefix = "union ";
                    else
                        prefix = "struct ";
                }

                identifier = prefix + s.get_name() + " " + identifier;
            },
            [&]( const enum_t& e )
            {
                if ( !trivial_type )
                    identifier = "enum " + e.get_name() + " " + identifier;
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
                        print_identifier( arg, arg_format, false );

                        identifier += arg_format + ",";
                    }

                    identifier.pop_back();
                }

                identifier += ")";
                print_identifier( f.get_return_type(), identifier, false );
            },
            [&]( const pointer_t& p )
            {
                identifier = "(*" + identifier + ")";

                const auto& pointee = type_db.lookup_type( p.get_elem_type() );
                if ( std::holds_alternative<function_t>( pointee ) ||
                     std::holds_alternative<array_t>( pointee ) )
                    identifier = "(*" + identifier + ")";
                else
                    identifier = "*" + identifier;

                print_identifier( p.get_elem_type(), identifier, false );
            },
            [&]( const array_t& arr )
            {
                if ( arr.is_fixed() )
                {
                    auto size = arr.get_array_size();
                    identifier += std::format( "[{}]", size );
                }
                else
                    identifier += std::format( "[]" );

                print_identifier( arr.get_elem_type(), identifier, false );
            },
            []( const auto&& a )
            {
                assert( false, "invalid type for identifier printer found" );
            } },
        type_data );
}
