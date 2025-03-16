#pragma once
#include <string>

using type_id = uint64_t;
using type_size_resolver = uint32_t( type_id );

class named_sized_type_t
{
public:
    virtual ~named_sized_type_t( ) = default;
    virtual std::string name_of( ) = 0;
    virtual size_t size_of( type_size_resolver& tr ) = 0;
};

class dependent_t
{
public:
    virtual ~dependent_t() = default;
    virtual std::vector<type_id> get_dependencies() = 0;
};
