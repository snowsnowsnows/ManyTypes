#pragma once
#include <string>
#include <utility>

#include "manytypes-lib/types/models/named_sized.h"

class null_type_t final : public named_sized_type_t
{
public:
    null_type_t() = default;

    std::string name_of() const override
    {
        assert( "attempted to retrieve name of for null type " );
        return "";
    }

    size_t size_of( type_size_resolver& tr ) const override
    {
        assert( "attempted to retrieve size for null type" );
        return 0;
    }
};

class basic_type_t final : public named_sized_type_t
{
public:
    constexpr basic_type_t( const std::string_view name, const size_t size )
        : name( name ), size( size ) {}

    std::string name_of() const override
    {
        return name;
    }

    size_t size_of( type_size_resolver& tr ) const override
    {
        return size;
    }

    std::string name;
    size_t size;
};

class array_t final : public named_sized_type_t
{
public:
    explicit array_t( const type_id id, const size_t array_size )
        : fixed_size( true )
        , size( array_size )
        , base( id )
    {
    }

    explicit array_t( const type_id id )
        : fixed_size( true )
        , size( 0 )
        , base( id )
    {
    }

    std::string name_of() const override
    {
        return "";
    }

    size_t size_of( type_size_resolver& tr ) const override
    {
        return fixed_size ? size * tr( base ) : sizeof( void* );
    }

    type_id get_elem_type() const
    {
        return base;
    }


    bool fixed_size;

    size_t size;
    type_id base;
};

class pointer_t final : public named_sized_type_t
{
public:
    explicit pointer_t( const type_id id )
        : base( id )
    {
    }

    std::string name_of() const override
    {
        return "";
    }

    size_t size_of( type_size_resolver& tr ) const override
    {
        return sizeof( void* );
    }

    type_id get_elem_type() const
    {
        return base;
    }

    type_id base;
};