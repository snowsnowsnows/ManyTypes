#pragma once
#include <string>

using type_id = uint64_t;
using type_size_resolver = size_t(*)( type_id );

class dependent_t
{
public:
    virtual ~dependent_t() = default;
    virtual std::vector<type_id> get_dependencies() const = 0;
};
