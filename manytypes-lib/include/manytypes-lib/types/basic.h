#pragma once
#include <string>
#include <utility>
#include <vector>

#include "manytypes-lib/types/models/named_sized.h"

namespace mt
{
class null_type_t final : public dependent_t
{
public:
    null_type_t() = default;

    std::vector<type_id> get_dependencies() const override
    {
        // assert( false && "cannot retrieve dependencies for null type" );
        return {};
    }
};

class qualified_t final : public dependent_t
{
public:
    qualified_t( const type_id underlying, const bool is_const, const bool is_volatile, const bool is_restrict )
        : underlying( underlying )
        , is_const( is_const )
        , is_volatile( is_volatile )
        , is_restrict( is_restrict )
    {
    }

    std::vector<type_id> get_dependencies() const override
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
        : name( name )
        , size( size )
    {
    }

    std::vector<type_id> get_dependencies() const override
    {
        return {};
    }

    std::string name;
    size_t size;
};

class array_t final : public dependent_t
{
public:
    explicit array_t( const type_id id, const size_t array_length, const size_t elem_size )
        : fixed_length( true )
        , length( array_length )
        , base( id )
        , element_size( elem_size )
    {
    }

    explicit array_t( const type_id id, const size_t elem_size )
        : fixed_length( true )
        , length( 0 )
        , base( id )
        , element_size( elem_size )
    {
    }

    bool is_fixed_length() const
    {
        return fixed_length;
    }

    size_t get_array_length() const
    {
        return length;
    }

    size_t get_elem_size() const
    {
        return element_size;
    }

    type_id get_elem_type() const
    {
        return base;
    }

    std::vector<type_id> get_dependencies() const override
    {
        return { base };
    }

private:
    bool fixed_length;

    size_t length;
    size_t element_size;
    type_id base;
};

class pointer_t final : public dependent_t
{
public:
    explicit pointer_t( const type_id id, uint32_t ptr_bit_size )
        : base( id )
        , ptr_bit_size( ptr_bit_size )
    {
    }

    type_id get_elem_type() const
    {
        return base;
    }

    std::vector<type_id> get_dependencies() const override
    {
        return { base };
    }

    uint32_t get_ptr_bit_size() const
    {
        return ptr_bit_size;
    }

private:
    type_id base;
    uint32_t ptr_bit_size;
};
} // namespace mt
