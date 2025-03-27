#include "manytypes-lib/formatter/x64dbg.h"

#include "manytypes-lib/util/util.h"
#include "nlohmann/json.hpp"

x64dbg_formatter::x64dbg_formatter( type_database_t db )
    : type_db( std::move( db ) ), anonymous_counter( 0 )
{
    json["enums"] = {};
    json["structUnions"] = {};
    json["types"] = {};
    json["functions"] = {};
}

nlohmann::json x64dbg_formatter::generate_json()
{
    std::unordered_set<type_id> visited;
    std::unordered_set<type_id> rec_stack;
    std::vector<type_id> sorted;

    std::function<void( type_id type, bool )> dfs_type;
    dfs_type = [&]( const type_id id, bool force_define_all )
    {
        assert( !rec_stack.contains( id ), "current dependency stack should not contain id. ciruclar dep" );
        if ( visited.contains( id ) )
            return;

        visited.insert( id );
        rec_stack.insert( id );

        std::unordered_set<type_id> layer_deps;

        type_id_data& type_info = type_db.lookup_type( id );
        std::visit(
            [&]( auto& a )
            {
                using T = std::decay_t<decltype( a )>;
                if constexpr ( std::is_same_v<T, structure_t> || std::is_same_v<T, enum_t> )
                {
                    // structures / enums require all members to be fully defined
                    // if we hit this then its a dependency
                    force_define_all = true;
                }
                else if constexpr ( std::is_same_v<T, elaborated_t> )
                {
                    // we must force underlying types to be dependencies of their underlying types
                    // because a struct may use a typedef which forward declares a type such as this
                    // typedef struct some_struct typedef_name

                    // however, if typedef_name is used within a struct
                    // this will cause a dependency on some_struct
                    // so it must be defined if we are already forcing dependencies
                    const elaborated_t& forwarder = a;
                    force_define_all |= forwarder.sugar.empty();
                }
                else if constexpr ( std::is_same_v<T, pointer_t> )
                {
                    force_define_all = false;
                }

                // todo maybe find another way of doing this without a massive or??
                // if another type depends on this type, it must be defined so we must visit it
                bool peek_type = std::is_same_v<T, pointer_t> ||
                                 std::is_same_v<T, typedef_type_t> ||
                                 std::is_same_v<T, function_t> ||
                                 std::is_same_v<T, qualified_t> ||
                                 std::is_same_v<T, array_t>;

                if ( peek_type || force_define_all )
                    layer_deps.insert_range( a.get_dependencies() );

                if ( !force_define_all )
                    visited.erase( id );
            },
            type_info );

        for ( const auto& dep : layer_deps )
            dfs_type( dep, force_define_all );

        rec_stack.erase( id );
        sorted.push_back( id );
    };

    for ( const auto& type_id : type_db.get_types() | std::views::keys )
    {
        if ( !visited.contains( type_id ) )
            dfs_type( type_id, false );
    }

    std::string out;
    for ( type_id id : sorted )
    {
        type_id_data& type_info = type_db.lookup_type( id );
        std::visit(
            overloads{
                [&]( const elaborated_t& e )
                {
                    elaborate_chain[id] = e.type;
                },
                [&]( pointer_t& p )
                {
                    nlohmann::json json;
                    json["name"] = insert_type_name( id, "" );
                    json["members"] = nlohmann::json::array();
                    json["size"] = type_db.bit_pointer_size;

                    nlohmann::json json_field;
                    json_field["bitSize"] = type_db.bit_pointer_size;
                    json_field["name"] = "ptr";
                    json_field["offset"] = 0;
                    json_field["type"] = lookup_type_name( p.get_elem_type() ) + "*";

                    json["members"].push_back( json_field );

                    json["structUnions"].push_back( json );
                },
                [&]( array_t& a )
                {
                    nlohmann::json json;
                    json["name"] = insert_type_name( id, "" );
                    json["members"] = nlohmann::json::array();
                    json["size"] = a.get_array_length() * a.get_elem_size();

                    nlohmann::json json_field;
                    json_field["bitSize"] = a.get_array_length() * a.get_elem_size();
                    json_field["name"] = "arr";
                    json_field["offset"] = 0;
                    json_field["bitfield"] = false;
                    json_field["arrsize"] = a.get_array_length();
                    json_field["type"] = lookup_type_name( a.get_elem_type() );

                    json["members"].push_back( json_field );

                    json["structUnions"].push_back( json );
                },
                [&]( basic_type_t& b )
                {
                    insert_type_name( id, b.name );
                },
                [&]( typedef_type_t& t )
                {
                    const type_id_data& underlying_type_data = type_db.lookup_type( id );
                    if ( std::holds_alternative<function_t>( underlying_type_data ) )
                    {
                        const auto& f = std::get<function_t>( underlying_type_data );

                        nlohmann::json json;
                        json["name"] = insert_type_name( id, t.alias );
                        json["rettype"] = lookup_type_name( f.get_return_type() );
                        json["args"] = nlohmann::json::array();

                        switch ( f.get_call_conv() )
                        {
                        case call_conv::cc_cdecl:
                            json["callconv"] = "cdecl";
                            break;
                        case call_conv::cc_stdcall:
                            json["callconv"] = "stdcall";
                            break;
                        case call_conv::cc_thiscall:
                            json["callconv"] = "thiscall";
                            break;
                        case call_conv::cc_fastcall:
                            json["callconv"] = "fastcall";
                            break;
                        }

                        for ( auto& arg : f.get_args() )
                        {
                            nlohmann::json json_arg;
                            json_arg["type"] = lookup_type_name( arg );
                            json_arg["name"] = "";

                            json["args"].push_back( json_arg );
                        }

                        json["functions"].push_back( json );
                    }
                    else
                    {
                        nlohmann::json json;
                        json["name"] = insert_type_name( id, t.alias );
                        json["type"] = out_type_names.at( t.type );

                        json["types"].push_back( json );
                    }
                },
                [&]( structure_t& s )
                {
                    nlohmann::json json;
                    json["name"] = insert_type_name( id, s.get_name() );
                    json["members"] = nlohmann::json::array();

                    auto& settings = s.get_settings();
                    json["size"] = settings.size;
                    json["isUnion"] = settings.is_union;

                    for ( auto& field : s.get_fields() )
                    {
                        nlohmann::json json_field;
                        json_field["bitSize"] = field.bit_size;
                        json_field["name"] = field.name.empty() ? lookup_type_name( field.type_id ) : field.name;
                        json_field["offset"] = field.bit_offset;
                        json_field["bitfield"] = field.is_bit_field;
                        json_field["type"] = lookup_type_name( field.type_id );

                        json["members"].push_back( json_field );
                    }

                    json["structUnions"].push_back( json );
                },
                [&]( enum_t& e )
                {
                    nlohmann::json json;
                    json["name"] = insert_type_name( id, e.get_name() );
                    json["members"] = nlohmann::json::array();
                    json["size"] = e.get_settings().size;
                    json["isFlags"] = true;

                    for ( const auto& [name, value] : e.get_members() )
                    {
                        nlohmann::json json_member;
                        json_member["name"] = value;
                        json_member["value"] = name;

                        json["members"].push_back( json_member );
                    }

                    json["enums"].push_back( json );
                },
                [&]( auto&& a )
                {
                    assert( false, "failed" );
                } },
            type_info );
    }

    return std::move( json );
}

std::string x64dbg_formatter::insert_type_name( const type_id id, const std::string& name )
{
    assert( !out_type_names.contains( id ), "map must not contain type id" );
    if ( name.empty() )
    {
        auto fmt = std::format( "__anonymous_type{}", anonymous_counter++ );
        out_type_names[id] = fmt;

        return fmt;
    }

    out_type_names[id] = name;
    return name;
}

std::string x64dbg_formatter::lookup_type_name( const type_id id )
{
    type_id underlying = id;
    while ( elaborate_chain.contains( underlying ) )
        underlying = elaborate_chain[underlying];

    assert( out_type_names.contains( underlying ), "type id must exist" );
    return out_type_names.at( underlying );
}