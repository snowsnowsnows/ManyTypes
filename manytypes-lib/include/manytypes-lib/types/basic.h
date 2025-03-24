#pragma once
#include <string>
#include <utility>

#include "manytypes-lib/types/models/named_sized.h"

class null_type_t final : public dependent_t
{
public:
    null_type_t() = default;

    std::vector<type_id> get_dependencies() override
    {
        assert( true, "cannot retreive dependencies for null type" );
        return {};
    }
};

class qualified_t final : public dependent_t
{
public:
    qualified_t( const type_id underlying, const bool is_const, const bool is_volatile, const bool is_restrict )
        : underlying( underlying ), is_const( is_const ), is_volatile( is_volatile ), is_restrict( is_restrict )
    {
    }

    std::vector<type_id> get_dependencies() override
    {
        return { underlying };
    }

    type_id underlying;

    bool is_const;
    bool is_volatile;
    bool is_restrict;
};

class basic_type_t final : public dependent_t
{
public:
    constexpr basic_type_t( const std::string_view name, const size_t size )
        : name( name ), size( size ) {}

    std::vector<type_id> get_dependencies() override
    {
        return {};
    }

    std::string name;
    size_t size;
};

class array_t final : public dependent_t
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

    bool is_fixed() const
    {
        return fixed_size;
    }

    size_t get_array_size() const
    {
        return size;
    }

    type_id get_elem_type() const
    {
        return base;
    }

    std::vector<type_id> get_dependencies() override
    {
        return { base };
    }

private:
    bool fixed_size;

    size_t size;
    type_id base;
};

class pointer_t final : public dependent_t
{
public:
    explicit pointer_t( const type_id id )
        : base( id )
    {
    }

    type_id get_elem_type() const
    {
        return base;
    }

    std::vector<type_id> get_dependencies() override
    {
        return { base };
    }

private:
    type_id base;
};