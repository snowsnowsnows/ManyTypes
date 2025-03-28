#pragma once
#include "manytypes-lib/db/database.h"
#include "manytypes-lib/db/clang-database.h"

namespace mt
{
struct clang_context_t
{
    explicit clang_context_t( const uint8_t byte_size_pointer )
        : type_db( byte_size_pointer ), clang_db( type_db ), failed( false )
    {
    }

    type_database_t type_db;
    clang_database_t clang_db;

    bool failed;
};
} // namespace mt