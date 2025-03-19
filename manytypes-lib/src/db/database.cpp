#include "manytypes-lib/db/database.h"
#include "manytypes-lib/util/util.h"

type_database_t::type_database_t()
{
    curr_type_id = 0;

    basic_type_t types[] = {
        { "bool", sizeof( bool ) * 8 },
        { "char", sizeof( char ) * 8 },
        { "unsigned char", sizeof( unsigned char ) * 8 },
        { "signed char", sizeof( signed char ) * 8 },
        { "wchar_t", sizeof( wchar_t ) * 8 },
        { "short", sizeof( short ) * 8 },
        { "unsigned short", sizeof( unsigned short ) * 8 },
        { "int", sizeof( int ) * 8 },
        { "unsigned int", sizeof( unsigned int ) * 8 },
        { "long", sizeof( long ) * 8 },
        { "unsigned long", sizeof( unsigned long ) * 8 },
        { "long long", sizeof( long long ) * 8 },
        { "unsigned long long", sizeof( unsigned long long ) * 8 },
        { "float", sizeof( float ) * 8 },
        { "double", sizeof( double ) * 8 },
        { "long double", sizeof( long double ) * 8 },
        { "void", 0 }
    };

    for ( auto& t : types )
        insert_type( t );
}

size_t type_database_t::size_of( const type_id id ) const
{
    assert( type_info.contains( id ), "type id is not present in type database" );
    // return size_of( type_info[ id ] );
    return 0;
}

std::string type_database_t::name_of( const type_id id ) const
{
    assert( type_info.contains( id ), "type id is not present in type database" );
    return name_of( type_info.at( id ) );
}

type_id type_database_t::insert_type( const type_id_data& data )
{
    std::visit(
        [&]( auto&& x )
        {
            using T = std::decay_t<decltype( x )>;
            if constexpr ( !std::is_same_v<T, array_t> && !std::is_same_v<T, pointer_t> &&
                           !std::is_same_v<T, null_type_t> &&
                           !std::is_same_v<T, alias_forwarder_t> )
            {
                auto name = name_of( x );
                if ( !name.empty() )
                {
                    // handle all other types
                    assert( !used_names.contains( name ), "type with current name exists" );
                    used_names.insert( name );
                }
            }

            type_info.insert( { curr_type_id, x } );
        },
        data );

    return curr_type_id++;
}

type_id type_database_t::insert_placeholder_type( const null_type_t& data )
{
    type_info.insert( { curr_type_id, data } );
    return curr_type_id++;
}

void type_database_t::update_type( const type_id id, const type_id_data& data )
{
    assert( type_info.contains( id ), "type with current id must exist" );
    type_info.at( id ) = data;
}

type_id_data type_database_t::lookup_type( const type_id id )
{
    assert( type_info.contains( id ), "type info must contain id" );
    return type_info.at( id );
}

type_id type_database_t::lookup_type_name( const char* name )
{
    // todo remove this assert because this should be allowed to fail
    assert( used_names.contains( name ), "name must be a used type name" );

    for ( auto& [id, data] : type_info )
    {
        auto found_name = name_of( data );
        if ( found_name == name )
            return id;
    }

    return 0;
}

bool type_database_t::contains_type( const type_id id ) const
{
    return type_info.contains( id );
}

const std::unordered_map<type_id, type_id_data>& type_database_t::get_types() const
{
    return type_info;
}