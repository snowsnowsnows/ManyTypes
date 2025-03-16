#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "manytypes-lib/types/models/named_sized.h"

class enum_t final : public named_sized_type_t, public dependent_t
{
public:
    explicit enum_t( const std::string& name, const type_id underlying_type )
        : enum_name( name ), underlying_type( underlying_type )
    {
    }

    std::string name_of( ) override
    {
        return enum_name;
    }

    size_t size_of( type_size_resolver& tr ) override
    {
        return tr( underlying_type );
    }

    bool insert_member( uint64_t value, const std::string& name )
    {
        for ( auto& member : members )
            assert( std::get<1>(member) != name, "member names must not be equal" );

        members.emplace_back( value, name );
        return true;
    }

    std::vector<type_id> get_dependencies( ) override
    {
        return { underlying_type };
    }

private:
    std::vector<std::pair<uint64_t, std::string>> members;

    std::string enum_name;
    type_id underlying_type;
};
