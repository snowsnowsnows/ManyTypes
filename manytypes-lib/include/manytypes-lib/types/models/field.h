#pragma once
#include <string>

namespace mt
{
using type_id = uint64_t;
struct base_field_t
{
    uint32_t bit_offset;
    uint32_t bit_size;
    bool is_bit_field;

    std::string name;
    type_id type_id;
};
} // namespace mt
