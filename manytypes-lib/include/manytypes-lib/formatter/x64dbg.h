#pragma once

#include "manytypes-lib/db/clang-database.h"

#include "manytypes-lib/types/alias.h"
#include "manytypes-lib/types/basic.h"
#include "manytypes-lib/types/enum.h"
#include "manytypes-lib/types/function.h"
#include "manytypes-lib/types/structure.h"

#include <sstream>
#include <utility>

#include <nlohmann/json.hpp>

class x64dbg_formatter
{
public:
    explicit x64dbg_formatter( type_database_t db );

    nlohmann::json generate_json( );

private:
    std::unordered_map<type_id, std::string> out_type_names;
    std::unordered_map<type_id, type_id> elaborate_chain;
    uint64_t anonymous_counter;

    type_database_t type_db;
    nlohmann::json json_db;

    std::string get_insert_type_name( type_id id, const std::string& name );
    std::string lookup_type_name( type_id id );

    std::pair<uint32_t, type_id> unfold_pointer_path( type_id id );
};