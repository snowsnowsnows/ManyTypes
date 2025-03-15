#pragma once
#include <string>
#include <utility>

#include "manytypes-lib/struct/models/named_sized.h"

class base_type_t final : public named_sized_type_t
{
public:
    base_type_t( std::string  name, size_t size )
        : name(std::move( name )), size( size )
    {
    }

    std::string name_of( ) override
    {
        return name;
    }

    size_t size_of( ) override
    {
        return size;
    }

    std::string name;
    size_t size;
};
