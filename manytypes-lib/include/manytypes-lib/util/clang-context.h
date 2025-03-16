#pragma once
#include "manytypes-lib/db/database.h"
#include "manytypes-lib/db/clang-database.h"

struct clang_context_t
{
    clang_context_t( )
        : type_db( { } ), clang_db( type_db ), failed( false )
    {
    }

    type_database_t type_db;
    clang_database_t clang_db;

    bool failed;
};
