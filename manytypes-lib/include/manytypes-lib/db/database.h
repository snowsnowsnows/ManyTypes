#pragma once
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <array>

#include "manytypes-lib/types/alias.h"
#include "manytypes-lib/types/basic.h"
#include "manytypes-lib/types/enum.h"
#include "manytypes-lib/types/function.h"
#include "manytypes-lib/types/structure.h"

namespace mt
{
using base_type_t = std::variant<function_t, array_t, pointer_t, basic_type_t>;

using type_id_data = std::variant<
    structure_t,
    enum_t,
    typedef_type_t,
    elaborated_t,
    qualified_t,
    function_t,
    array_t,
    pointer_t,
    basic_type_t,

    null_type_t>;

class type_database_t
{
public:
    explicit type_database_t( uint8_t byte_pointer_size );

    type_id insert_type( const type_id_data& data, type_id semantic_parent = 0 );
    type_id insert_placeholder_type( const null_type_t& data, type_id semantic_parent = 0 );

    void update_type( type_id id, const type_id_data& data );

    type_id_data& lookup_type( type_id id );

    void insert_semantic_parent( type_id id, type_id parent );

    bool contains_type( type_id id ) const;
    const std::unordered_map<type_id, type_id_data>& get_types() const;

    std::string get_type_print( type_id id );

    static std::array<basic_type_t, 17> types;
    uint8_t bit_pointer_size;

private:
    std::unordered_map<type_id, type_id_data> type_info;
    std::unordered_map<type_id, type_id> type_scopes;

    type_id curr_type_id;
};
} // namespace mt
