#pragma once

#include "manytypes-lib/db/clang-database.h"

#include "manytypes-lib/types/alias.h"
#include "manytypes-lib/types/basic.h"
#include "manytypes-lib/types/enum.h"
#include "manytypes-lib/types/function.h"
#include "manytypes-lib/types/structure.h"

#include <sstream>

class c_language_formatter
{
public:
    std::stringstream print_structure( structure_t& s );
    std::stringstream print_enum( enum_t& e );

    std::stringstream print_forward_alias( alias_type_t& );
    std::stringstream print_function( function_t& f );

    std::stringstream print_arr( array_t& a );
    std::stringstream print_ptr( pointer_t& ptr );

private:
    type_database_t type_db;
    clang_database_t clang_db;

    void print_identifier( type_id_data& type, std::string& identifier );
};