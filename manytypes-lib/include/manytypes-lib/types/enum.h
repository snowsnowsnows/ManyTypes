#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "manytypes-lib/types/models/named_sized.h"

struct enum_settings
{
    std::string name;

    type_id underlying;

    bool is_forward;
};

class enum_t final : public dependent_t
{
public:
    explicit enum_t( const enum_settings& settings )
        : settings( settings )
    {
    }

    bool insert_member( uint64_t value, const std::string& name )
    {
        for ( auto& member : members )
            assert( std::get<1>( member ) != name, "member names must not be equal" );

        members.emplace_back( value, name );
        return true;
    }

    enum_settings& get_settings()
    {
        return settings;
    }

    std::string get_name() const
    {
        return settings.name;
    }

    const std::vector<std::pair<uint64_t, std::string>>& get_members() const
    {
        return members;
    }

    std::vector<type_id> get_dependencies() const override
    {
        return { settings.underlying };
    }

private:
    std::vector<std::pair<uint64_t, std::string>> members;

    enum_settings settings;
};
