#pragma once
#include "manytypes-lib/struct/models/named_sized.h"

class array_t final : public named_sized_type_t
{
public:
    explicit array_t( const type_id id, const size_t array_size )
        : fixed_size( true ), size( array_size ), base( id )
    {
    }

    explicit array_t( const type_id id )
        : fixed_size( true ), size( 0 ), base( id )
    {
    }

    std::string name_of( ) override;
    size_t size_of( ) override;

    type_id get_elem_type( ) const
    {
        return base;
    }

private:
    bool fixed_size;
    size_t size;

    type_id base;
};

class pointer_t final : public named_sized_type_t
{
public:
    explicit pointer_t( const type_id id )
        :  base( id )
    {
    }

    std::string name_of( ) override;
    size_t size_of( ) override;

    type_id get_elem_type( ) const
    {
        return base;
    }

private:
    type_id base;
};

