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
    explicit formatter_clang( type_database_t db )
        : type_db( std::move( db ) )
    {
    }

    std::string print_database();

private:
    type_database_t type_db;

    std::string print_forwards();
    std::string print_typedefs();
    std::string print_enums();
    std::string print_structs();

    void print_identifier( const type_id& type, std::string& identifier );

    std::string print_type( type_id id, bool ignore_anonymous = false );
    std::string print_structure( structure_t& s );
    std::string print_enum( const enum_t& e );
    std::string print_forward_alias( const typedef_type_t& a );
};
} // namespace mt
