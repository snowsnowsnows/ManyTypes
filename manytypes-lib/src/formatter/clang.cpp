#include "manytypes-lib/formatter/clang.h"
#include "manytypes-lib/util/util.h"

#include <functional>
#include <ranges>

void gather_all_dependencies( const type_id& id,
    type_database_t& type_db,
    std::unordered_set<type_id>& visited,
    std::vector<type_id>& all_deps )
{
    if ( visited.contains( id ) )
        return;

    visited.insert( id );
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
                deps = p.get_dependencies();
            },
            [&]( auto&& )
            {
                // Trivial type
            } },
        type_info );

    for ( const auto& dep_id : deps )
    {
        if ( !visited.contains( dep_id ) )
        {
            all_deps.push_back( dep_id );
            gather_all_dependencies( dep_id, type_db, visited, all_deps );
        }
    }
}

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
        gather_all_dependencies( id, type_db, visited, deps );

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
    {
        auto result = print_type( id );
        if ( !result.empty() )
            out += result + ";\n";
    }

    return out;
}

std::string formatter_clang::print_structure( const structure_t& s )
{
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
                // assert( false, "unknown type visited" );
                return "";
            } },
        type_db.lookup_type( id ) );
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

                identifier = std::format( "{} {}{}", f.sugar, f.scope, type_print ) + identifier;
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
                if ( std::holds_alternative<function_t>( pointee ) ||
                     std::holds_alternative<array_t>( pointee ) )
                    identifier = "(*" + identifier + ")";
                else
                    identifier = "*" + identifier;

                print_identifier( p.get_elem_type(), identifier );
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

                print_identifier( arr.get_elem_type(), identifier );
            },
            []( const auto& a )
            {
                assert( false, "invalid type for identifier printer found" );
            } },
        type_data );
}

bool formatter_clang::is_type_anonymous( const type_id type )
{
    auto type_data = type_db.lookup_type( type );
    return std::visit(
        overloads{
            [&]( const qualified_t& q )
            {
                return is_type_anonymous( q.underlying );
            },
            [&]( const elaborated_t& f )
            {
                return is_type_anonymous( f.type );
            },

            [&]( const structure_t& s )
            {
                return s.get_name() == "";
            },
            [&]( const enum_t& e )
            {
                return e.get_name() == "";
            },

            []( const auto& a )
            {
                return false;
            } },
        type_data );
}
