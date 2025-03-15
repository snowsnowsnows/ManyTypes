#pragma once
#include <string>

using type_id = uint64_t;
class named_sized_type_t
{
public:
    virtual ~named_sized_type_t() = default;

    virtual std::string name_of() = 0;
    virtual size_t size_of() = 0;
};
