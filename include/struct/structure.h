#pragma once
#include <algorithm>
#include <vector>

#include "../members/field.h"
#include "models/params.h"
#include "models/named_sized.h"

class structure_t final : public structure_settings, public named_sized_type_t
{
public:
    virtual ~structure_t( ) = default;

    explicit structure_t( const structure_settings& settings, const bool in_order_insertion )
        : structure_settings( settings ), in_order_insert( in_order_insertion )
    {
    }

    virtual bool add_field( const base_field_t& field );
    [[nodiscard]] bool is_field_valid( const base_field_t& next_field );

    bool finalize( )
    {
    };

    [[nodiscard]] const std::vector<base_field_t>& get_fields( ) const
    {
        return s_fields;
    }

    size_t size_of( ) override;
    std::string name_of( ) override;

protected:
    std::vector<base_field_t> s_fields;
    bool in_order_insert;
};
