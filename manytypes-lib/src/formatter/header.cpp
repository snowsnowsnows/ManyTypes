#include "manytypes-lib/formatter/header.h"

#include "manytypes-lib/util/util.h"

std::stringstream c_language_formatter::print_structure( structure_t& s )
{
    std::stringstream out;
    out << std::format( "struct {} {", s.name_of() );

    for ( const auto& field : s.get_fields() )
    {
        std::string identifier;

        auto type = type_db.lookup_type( field.type_id );
        print_identifier( type, identifier );

        if ( !field.is_bit_field )
            out << std::format( "\n{};", identifier );
        else
            out << std::format( "\n{} : {};", identifier, field.bit_size );
    }

    out << "\n};";

    return out;
}

std::stringstream c_language_formatter::print_enum( enum_t& e )
{
}

std::stringstream c_language_formatter::print_forward_alias( alias_type_t& a )
{
    std::stringstream out;
    out << std::format( "typedef {} {};", type_db.name_of( a.type ), a.alias );

    return out;
}

std::stringstream c_language_formatter::print_function( function_t& f )
{
}

std::stringstream c_language_formatter::print_arr( array_t& a )
{
}

std::stringstream c_language_formatter::print_ptr( pointer_t& ptr )
{
}

void c_language_formatter::print_identifier( type_id_data& type, std::string& identifier )
{
    std::visit(
        overloads{
            // base
            [&]( const alias_type_t& a )
            {
                identifier = a.alias + " " + identifier;
            },
            [&]( const basic_type_t& b )
            {
                identifier = b.name + " " + identifier;
            },

            // complex types
            [&]( const function_t& f )
            {
                identifier += "(";

                if ( !f.args.empty() )
                {
                    for ( const auto& arg : f.args )
                    {
                        std::string arg_format;
                        type_id_data type_data = type_db.lookup_type( arg );
                        print_identifier( type_data, arg_format );

                        identifier += arg_format + ",";
                    }

                    identifier.pop_back();
                }

                identifier += ")";

                type_id_data return_type_data = type_db.lookup_type( f.return_type );
                print_identifier( return_type_data, identifier );
            },
            [&]( const pointer_t& p )
            {
                identifier = "(*" + identifier + ")";

                auto pointee = type_db.lookup_type( p.base );
                if ( std::holds_alternative<function_t>( pointee ) )
                    identifier = "(*" + identifier + ")";
                else
                    identifier = "*" + identifier;

                print_identifier( pointee, identifier );
            },
            [&]( const array_t& arr )
            {
                if ( arr.fixed_size )
                {
                    auto size = arr.size;
                    identifier += std::format( "[{}]", size );
                }
                else
                    identifier += std::format( "[]" );

                auto next = type_db.lookup_type( arr.base );
                print_identifier( next, identifier );
            },
            []( auto&& )
            {
                assert( true, "invalid type for identifier printer found" );
            } },
        type );
}
