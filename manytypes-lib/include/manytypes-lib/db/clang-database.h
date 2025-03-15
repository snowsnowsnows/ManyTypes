#pragma once
#include "manytypes-lib/db/database.h"
#include "manytypes-lib/util/clang-utils.h"

class clang_database_t
{
public:
    explicit clang_database_t( type_database_t& type_db )
        : type_db( &type_db )
    {
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

private:
    std::unordered_map<CXType, type_id, cx_type_hash, cx_type_equal> type_map;
    type_database_t* type_db;
};
