#pragma once
#include <cstdint>
#include <string>

struct structure_settings
{
    std::string name;

    uint32_t align;
    uint32_t size;

    bool is_union;
};
