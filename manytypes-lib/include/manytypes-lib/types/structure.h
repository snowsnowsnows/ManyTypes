#pragma once
#include <algorithm>
#include <utility>
#include <vector>
#include <unordered_set>

#include "manytypes-lib/types/models/field.h"
#include "manytypes-lib/types/models/named_sized.h"

namespace mt
{
struct structure_settings
{
    std::string name;

    uint32_t align;
    uint32_t size;

    bool is_union;
    bool is_forward;
};

class structure_t final : public dependent_t
{
public:
    ~structure_t() override = default;

    explicit structure_t( structure_settings settings, const bool in_order_insertion )
        : settings( std::move( settings ) ), in_order_insert( in_order_insertion )
    {
    }

    bool add_field( const base_field_t& field );

    [[nodiscard]] std::vector<base_field_t>& get_fields()
    {
        return s_fields;
    }

    bool is_union() const
    {
        return settings.is_union;
    }

    std::string get_name() const
    {
        return settings.name;
    }

    std::vector<type_id> get_dependencies() const override
    {
        std::unordered_set<type_id> deps;
        for ( auto& field : s_fields )
            deps.insert( field.type_id );

        return std::vector( deps.begin(), deps.end() );
    }

    structure_settings& get_settings()
    {
        return settings;
    }

protected:
    structure_settings settings;

    std::vector<base_field_t> s_fields;
    bool in_order_insert;
};
} // namespace mt
