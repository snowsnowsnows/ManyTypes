#include "manytypes-lib/formatter/x64dbg.h"

#include "manytypes-lib/util/util.h"
#include "nlohmann/json.hpp"

x64dbg_formatter::x64dbg_formatter( type_database_t db )
    : anonymous_counter( 0 )
    , type_db( std::move( db ) )
{
    json_db["enums"] = nlohmann::json::array( );
    json_db["structUnions"] = nlohmann::json::array( );
    json_db["types"] = nlohmann::json::array( );
    json_db["functions"] = nlohmann::json::array( );
}

nlohmann::json x64dbg_formatter::generate_json( )
{
    std::unordered_set<type_id> visited;
    std::unordered_set<type_id> rec_stack;
    std::vector<type_id> sorted;

    std::function<void( type_id type, bool )> dfs_type;
    dfs_type = [&] ( const type_id id, bool force_define_all )
    {
        if ( visited.contains( id ) || rec_stack.contains( id ) )
            return;

        visited.insert( id );
        rec_stack.insert( id );

        std::unordered_set<type_id> layer_deps;
        std::visit(
            [&] ( auto& a )
            {
                layer_deps.insert_range( a.get_dependencies( ) );
            },
            type_db.lookup_type( id ) );

        for ( const auto& dep : layer_deps )
            dfs_type( dep, force_define_all );

        rec_stack.erase( id );
        sorted.push_back( id );
    };

    for ( const auto& type_id : type_db.get_types( ) | std::views::keys )
    {
        if ( !visited.contains( type_id ) )
            dfs_type( type_id, false );
    }

    // initialize names and basic information
    for ( type_id id : sorted )
    {
        std::visit(
            overloads{
                [&] ( qualified_t& q )
                {
                    elaborate_chain[id] = q.underlying;
                },
                [&] ( elaborated_t& e )
                {
                    elaborate_chain[id] = e.type;
                },
                [&] ( basic_type_t& b )
                {
                    get_insert_type_name( id, b.name );
                },
                [&] ( pointer_t& p )
                {
                    get_insert_type_name( id, "" );
                },
                [&] ( function_t& f )
                {
                    get_insert_type_name( id, "" );
                },
                [&] ( array_t& a )
                {
                    get_insert_type_name( id, "" );
                },
                [&] ( typedef_type_t& t )
                {
                    get_insert_type_name( id, t.alias );
                },
                [&] ( structure_t& s )
                {
                    get_insert_type_name( id, s.get_name( ) );
                },
                [&] ( enum_t& e )
                {
                    get_insert_type_name( id, e.get_name( ) );
                },
                [&] ( auto&& a )
                {
                    // assert( false, "failed" );
                } },
            type_db.lookup_type( id ) );
    }

    for ( type_id id : sorted )
    {
        std::visit(
            overloads{
                [&] ( pointer_t& p )
                {
                    nlohmann::json json;
                    json["name"] = get_insert_type_name( id, "" );
                    json["members"] = nlohmann::json::array( );
                    json["bitSize"] = type_db.bit_pointer_size;

                    nlohmann::json json_field;
                    json_field["bitSize"] = type_db.bit_pointer_size;
                    json_field["name"] = "ptr";
                    json_field["offset"] = 0;

                    auto ptr_size = p.get_ptr_bit_size( );
                    if ( ptr_size != type_db.bit_pointer_size )
                    {
                        // x64dbg wont be able to read the pointer anyways, so we can break the chain
                        // and write in a non pointer type

                        switch ( ptr_size )
                        {
                        case 32:
                            json_field["type"] = "unsigned int";
                            break;
                        case 64:
                            json_field["type"] = "unsigned long long";
                            break;
                        default:
                            assert( false, "invalid pointer size detected" );
                        }
                    }
                    else
                        json_field["type"] = lookup_type_name( p.get_elem_type( ) ) + "*";

                    json["members"].push_back( json_field );

                    json_db["structUnions"].push_back( json );
                },
                [&] ( array_t& a )
                {
                    nlohmann::json json;
                    json["name"] = get_insert_type_name( id, "" );
                    json["members"] = nlohmann::json::array( );
                    json["bitSize"] = a.get_array_length( ) * a.get_elem_size( );

                    nlohmann::json json_field;
                    json_field["bitSize"] = a.get_array_length( ) * a.get_elem_size( );
                    json_field["name"] = "arr";
                    json_field["offset"] = 0;
                    json_field["bitfield"] = false;
                    json_field["arrsize"] = a.get_array_length( );

                    json_field["type"] = lookup_type_name( a.get_elem_type( ) );

                    json["members"].push_back( json_field );

                    json_db["structUnions"].push_back( json );
                },
                [&] ( typedef_type_t& t )
                {
                    const type_id_data& underlying_type_data = type_db.lookup_type( id );
                    if ( std::holds_alternative<function_t>( underlying_type_data ) )
                    {
                        const auto& f = std::get<function_t>( underlying_type_data );
                        get_insert_type_name( id, t.alias );
                    }
                    else
                    {
                        nlohmann::json json;
                        json["name"] = get_insert_type_name( id, t.alias );
                        json["type"] = lookup_type_name( t.type );

                        if ( json["name"] == json["type"] )
                            return;

                        json_db["types"].push_back( json );
                    }
                },
                [&] ( function_t& f )
                {
                    nlohmann::json json;
                    json["name"] = get_insert_type_name( id, "" );
                    json["rettype"] = lookup_type_name( f.get_return_type( ) );
                    json["args"] = nlohmann::json::array( );

                    switch ( f.get_call_conv( ) )
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

                    for ( auto& arg : f.get_args( ) )
                    {
                        nlohmann::json json_arg;
                        json_arg["type"] = lookup_type_name( arg );
                        json_arg["name"] = "";

                        json["args"].push_back( json_arg );
                    }

                    json_db["functions"].push_back( json );
                },
                [&] ( structure_t& s )
                {
                    nlohmann::json json;
                    json["name"] = get_insert_type_name( id, s.get_name( ) );
                    json["members"] = nlohmann::json::array( );

                    auto& settings = s.get_settings( );
                    json["bitSize"] = settings.size;
                    json["isUnion"] = settings.is_union;

                    for ( const auto& field : s.get_fields( ) )
                    {
                        nlohmann::json json_field;
                        json_field["name"] = field.name.empty( ) && !field.is_bit_field ? lookup_type_name( field.type_id ) : field.name;
                        json_field["bitSize"] = field.bit_size;
                        json_field["bitOffset"] = field.bit_offset;
                        json_field["bitfield"] = field.is_bit_field;
                        json_field["type"] = lookup_type_name( field.type_id );

                        json["members"].push_back( json_field );
                    }

                    json_db["structUnions"].push_back( json );
                },
                [&] ( enum_t& e )
                {
                    nlohmann::json json;
                    json["name"] = get_insert_type_name( id, e.get_name( ) );
                    json["members"] = nlohmann::json::array( );
                    json["bitSize"] = e.get_settings( ).size;
                    json["isFlags"] = true;

                    for ( const auto& [val, name] : e.get_members( ) )
                    {
                        nlohmann::json json_member;
                        json_member["name"] = name;
                        json_member["value"] = static_cast<int64_t>(val);

                        json["members"].push_back( json_member );
                    }

                    json_db["enums"].push_back( json );
                },
                [&] ( auto&& a )
                {
                    // assert( false, "failed" );
                } },
            type_db.lookup_type( id ) );
    }

    return json_db;
}

std::string x64dbg_formatter::get_insert_type_name( const type_id id, const std::string& name )
{
    if ( out_type_names.contains( id ) )
        return out_type_names.at( id );

    if ( name.empty( ) )
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
    auto [ptr_depth, underlying] = unfold_pointer_path( id );
    while ( elaborate_chain.contains( underlying ) )
        underlying = elaborate_chain.at( underlying );

    assert( out_type_names.contains( underlying ), "type id must exist" );
    return out_type_names.at( underlying ) + std::string( ptr_depth, '*' );
}

std::pair<uint32_t, type_id> x64dbg_formatter::unfold_pointer_path( const type_id id )
{
    uint32_t pointer_depth = 0;
    type_id underlying_type = id;

    while ( true )
    {
        while ( elaborate_chain.contains( underlying_type ) )
            underlying_type = elaborate_chain.at( underlying_type );

        type_id_data& data = type_db.lookup_type( underlying_type );
        if ( std::holds_alternative<pointer_t>( data ) )
        {
            auto& ptr = std::get<pointer_t>( data );
            if ( ptr.get_ptr_bit_size( ) != type_db.bit_pointer_size )
                break; // redirect to base pointer structure

            underlying_type = ptr.get_elem_type( );
            pointer_depth++;
        }
        else
            break;
    }

    return { pointer_depth, underlying_type };
}