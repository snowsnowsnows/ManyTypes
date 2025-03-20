#include "manytypes-lib/db/database.h"
#include "manytypes-lib/util/util.h"

type_database_t::type_database_t()
{
    curr_type_id = 1;

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
    type_info.insert( { curr_type_id, data } );
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

bool type_database_t::contains_type( const type_id id ) const
{
    return type_info.contains( id );
}

const std::unordered_map<type_id, type_id_data>& type_database_t::get_types() const
{
    return type_info;
}