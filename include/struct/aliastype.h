#pragma once

#include "members/field.h"
#include "struct/models/sized.h"

class alias_type_t final : public named_sized_type_t
{
public:
    alias_type_t( std::string alias, const type_id type )
        : alias( std::move( alias ) ), type( type )
    {
    }

    std::string name_of( ) override
    {
        return alias;
    }

    size_t size_of( ) override
    {
        assert( true, "unable to look up size of alias for type" );
        return 0;
    }

    std::string alias;
    type_id type;
};
