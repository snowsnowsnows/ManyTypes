#pragma once

#include "manytypes-lib/db/clang-database.h"

#include "manytypes-lib/types/alias.h"
#include "manytypes-lib/types/basic.h"
#include "manytypes-lib/types/enum.h"
#include "manytypes-lib/types/function.h"
#include "manytypes-lib/types/structure.h"

#include <sstream>
#include <utility>

namespace mt
{
class formatter_clang
{
public:
    explicit formatter_clang( type_database_t& db, bool include_offsets = false, const std::string& tab_str = "\t" )
        : type_db( db ), include_offsets( include_offsets ), tab_str( tab_str )
    {
    }

    std::string print_database();

    std::string print_type( type_id id, uint8_t indent = 0, bool ignore_anonymous = false );
    std::string print_structure( structure_t& s, uint8_t indent = 0 );
    std::string print_enum( const enum_t& e, uint8_t indent = 0 );
    std::string print_forward_alias( const typedef_type_t& a, uint8_t indent = 0 );

private:
    type_database_t& type_db;
    bool include_offsets = false;
    std::string tab_str;

    std::string print_forwards();
    std::string print_typedefs();
    std::string print_enums();
    std::string print_structs();

    void print_identifier( const type_id& type, std::string& identifier, uint8_t indent );

    std::string create_indents( const uint8_t indents );
};
} // namespace mt
