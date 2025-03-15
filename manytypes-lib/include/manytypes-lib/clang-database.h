#pragma once
#include "database.h"
#include "clang-utils.h"

class clang_database_t
{
public:
    clang_database_t( )
    {
    }

    static clang_database_t& get( )
    {
        static clang_database_t inst = { };
        return inst;
    }

    type_id get_create_type_id( CXType type )
    {
        if ( type_map.contains( type ) )
            return type_map[ type ];

        auto id = type_database_t::
    }

    void save_type_id( const CXType& type, const type_id id )
    {
        type_map.insert( { type, id } );
    }

    type_id get_type_id( const CXType& type ) const
    {
        // must assert that the type exists
        return type_map.at( type );
    }

    bool is_type_defined( const CXType& type ) const
    {
        return type_map.contains( type );
    }

    bool

private:
    std::unordered_map<CXType, type_id, cx_type_hash, cx_type_equal> type_map;
};
