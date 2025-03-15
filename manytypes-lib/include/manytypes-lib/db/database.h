#pragma once
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "struct/alias.h"
#include "struct/base.h"
#include "struct/array.h"
#include "struct/enum.h"
#include "struct/function.h"
#include "struct/structure.h"

using type_id_data = std::variant<
    structure_t, enum_t,
    function_t, array_t, pointer_t,
    alias_type_t,

    base_type_t
>;

class type_database_t
{
public:
    type_database_t( )
    {
        curr_type_id = 0;

        constexpr base_type_t types[ ] = {
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

    size_t size_of( const type_id id )
    {
        assert( type_info.contains(id), "type id is not present in type database" );
        return size_of( type_info[ id ] );
    }

    std::string name_of( const type_id id )
    {
        assert( type_info.contains(id), "type id is not present in type database" );
        return name_of( type_info[ id ] );
    }

    type_id insert_type( const type_id_data& data )
    {
        assert( !used_names.contains( name_of(data) ), "type with current name exists" );

        type_info.insert( { curr_type_id, data } );
        used_names.insert( name_of( curr_type_id ) );

        return curr_type_id++;
    }

    type_id_data lookup_type( const type_id id )
    {
        assert( type_info.contains(id), "type info must contain id" );
        return type_info[ id ];
    }

    bool contains_type( const type_id id )
    {
        return type_info.contains( id );
    }

private:
    std::unordered_set<std::string> used_names;
    std::unordered_map<type_id, type_id_data> type_info;

    type_id curr_type_id;

    static size_t size_of( const type_id_data& id )
    {
        return std::visit(
            [] ( const auto& obj )
            {
                return obj.size_of( );
            }, id );
    }

    static std::string name_of( const type_id_data& id )
    {
        return std::visit(
            [] ( const auto& obj )
            {
                return obj.name_of( );
            }, id );
    }
};
