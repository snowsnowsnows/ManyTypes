#include "manytypes-lib/db/database.h"
#include "manytypes-lib/util/util.h"

std::array<basic_type_t, 17> type_database_t::types = {
    basic_type_t{ "bool", sizeof( bool ) * 8 },
    basic_type_t{ "char", sizeof( char ) * 8 },
    basic_type_t{ "unsigned char", sizeof( unsigned char ) * 8 },
    basic_type_t{ "signed char", sizeof( signed char ) * 8 },
    basic_type_t{ "wchar_t", sizeof( wchar_t ) * 8 },
    basic_type_t{ "short", sizeof( short ) * 8 },
    basic_type_t{ "unsigned short", sizeof( unsigned short ) * 8 },
    basic_type_t{ "int", sizeof( int ) * 8 },
    basic_type_t{ "unsigned int", sizeof( unsigned int ) * 8 },
    basic_type_t{ "long", sizeof( long ) * 8 },
    basic_type_t{ "unsigned long", sizeof( unsigned long ) * 8 },
    basic_type_t{ "long long", sizeof( long long ) * 8 },
    basic_type_t{ "unsigned long long", sizeof( unsigned long long ) * 8 },
    basic_type_t{ "float", sizeof( float ) * 8 },
    basic_type_t{ "double", sizeof( double ) * 8 },
    basic_type_t{ "long double", sizeof( long double ) * 8 },
    basic_type_t{ "void", 0 }
};

type_database_t::type_database_t( )
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

type_id type_database_t::insert_type( const type_id_data& data, type_id semantic_parent )
{
    assert( semantic_parent == 0 || type_scopes.contains(semantic_parent), "semantic parent must be valid type" );

    type_info.insert( { curr_type_id, data } );
    if ( semantic_parent )
        type_scopes[curr_type_id] = semantic_parent;

    return curr_type_id++;
}

type_id type_database_t::insert_placeholder_type( const null_type_t& data, type_id semantic_parent )
{
    // todo this is a place holder for the insert place holder
    return insert_type( data, semantic_parent );
}

void type_database_t::insert_semantic_parent( type_id id, type_id parent )
{
    assert( type_info.contains( id ), "type must exist" );
    assert( type_info.contains( parent ), "parent type must exist" );
    assert( !type_scopes.contains( id ), "type must not exist in scope" );

    type_scopes.insert( { id, parent } );
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

const std::unordered_map<type_id, type_id_data>& type_database_t::get_types( ) const
{
    return type_info;
}