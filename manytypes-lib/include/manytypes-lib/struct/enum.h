#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "manytypes-lib/struct/models/named_sized.h"

class enum_t final : public named_sized_type_t
{
public:
    explicit enum_t( const type_id underlying_type )
        : underlying_type( underlying_type )
    {
    }

    size_t size_of( ) override;
    std::string name_of( ) override;

    bool insert_member( uint64_t value, const std::string& name )
    {
        for ( auto& member : members )
            assert( std::get<1>(member) != name, "member names must not be equal" );

        members.emplace_back( value, name );
        return true;
    }

private:
    std::vector<std::pair<uint64_t, std::string>> members;
    type_id underlying_type;
};
