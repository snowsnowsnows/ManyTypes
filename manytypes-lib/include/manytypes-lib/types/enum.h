#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "manytypes-lib/types/models/named_sized.h"

class enum_t final : public dependent_t
{
public:
    explicit enum_t( std::string name, const type_id underlying_type )
        : enum_name( std::move( name ) ), underlying_type( underlying_type )
    {
    }

    bool insert_member( uint64_t value, const std::string& name )
    {
        for ( auto& member : members )
            assert( std::get<1>( member ) != name, "member names must not be equal" );

        members.emplace_back( value, name );
        return true;
    }

    std::string get_name() const
    {
        return enum_name;
    }

    const std::vector<std::pair<uint64_t, std::string>>& get_members() const
    {
        return members;
    }

    std::vector<type_id> get_dependencies() override
    {
        return { underlying_type };
    }

private:
    std::vector<std::pair<uint64_t, std::string>> members;

    std::string enum_name;
    type_id underlying_type;
};
