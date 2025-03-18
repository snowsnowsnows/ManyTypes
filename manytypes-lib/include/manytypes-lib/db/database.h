#pragma once
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "manytypes-lib/types/alias.h"
#include "manytypes-lib/types/basic.h"
#include "manytypes-lib/types/enum.h"
#include "manytypes-lib/types/function.h"
#include "manytypes-lib/types/structure.h"

using base_type_t = std::variant<function_t, array_t, pointer_t, basic_type_t>;

using type_id_data = std::variant<
    structure_t, enum_t,
    alias_type_t, alias_forwarder_t,
    function_t, array_t, pointer_t, basic_type_t,

    null_type_t
>;

class type_database_t
{
public:
    type_database_t( );

    size_t size_of( type_id id ) const;
    std::string name_of( type_id id ) const;

    type_id insert_type( const type_id_data& data );
    type_id insert_placeholder_type( const null_type_t& data );

    void update_type( type_id id, const type_id_data& data );

    type_id_data lookup_type( type_id id );
    type_id lookup_type_name( const char* name );

    bool contains_type( type_id id ) const;
    const std::unordered_map<type_id, type_id_data>& get_types( ) const;

private:
    std::unordered_set<std::string> used_names;
    std::unordered_map<type_id, type_id_data> type_info;

    type_id curr_type_id;

    //size_t size_of( const type_id_data& id )
    //{
    //    // todo not good.... cause crash on circular dependency
    //    auto res = [&] ( type_id s_id ) -> size_t
    //    {
    //        return size_of( type_info.at( s_id ) );
    //    };
    //
    //    return std::visit(
    //        [&res] ( const auto& obj )
    //        {
    //            return obj.size_of( res );
    //        }, id );
    //}

    static std::string name_of( const type_id_data& id )
    {
        return std::visit(
            [] ( const auto& obj )
            {
                return obj.name_of( );
            },
            id );
    }
};