#pragma once

#include "manytypes-lib/db/clang-database.h"

#include "manytypes-lib/types/alias.h"
#include "manytypes-lib/types/basic.h"
#include "manytypes-lib/types/enum.h"
#include "manytypes-lib/types/function.h"
#include "manytypes-lib/types/structure.h"

#include <sstream>
#include <utility>

class x64dbg_formatter
{
public:
    explicit x64dbg_formatter( type_database_t db )
        : type_db( std::move( db ) )
    {
    }

    std::string generate_json();

private:
    type_database_t type_db;
};