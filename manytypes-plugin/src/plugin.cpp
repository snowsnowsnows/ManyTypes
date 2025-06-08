#include "plugin.h"

#include <filesystem>
#include <queue>
#include <string>
#include <mutex>

#include "manytypes-lib/manytypes.h"
#include "manytypes-lib/formatter/clang.h"
#include "manytypes-lib/util/util.h"

std::atomic<const char*> g_curr_image_name;

std::mutex g_typedb_mutex;
std::optional<mt::type_database_t> g_typedb = std::nullopt;

std::unordered_map<std::string, std::filesystem::file_time_type> last_save_time;

bool create_file( const std::filesystem::path& path, const bool hidden = false )
{
    create_directories( path.parent_path() );

    if ( !exists( path ) )
    {
        CloseHandle( CreateFileW( path.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_NEW,
            ( hidden ? FILE_ATTRIBUTE_HIDDEN : 0 ) | FILE_ATTRIBUTE_NORMAL,
            nullptr ) );

        return true;
    }

    return false;
}

void plugin_run_loop()
{
    const char* curr_image = g_curr_image_name;
    if ( curr_image == nullptr )
        return;

    std::filesystem::path image_path( curr_image );

    std::string norm_image_name = image_path.stem().string();
    std::ranges::replace( norm_image_name, ' ', '-' );

    const auto run_root = std::filesystem::current_path();

    const auto manytypes_root = run_root / "ManyTypes";
    const auto manytypes_artifacts = manytypes_root / ".artifacts";

    const auto dbg_workspace = manytypes_root / norm_image_name;
    const auto dbg_workspace_root = dbg_workspace / "project.h";

    if ( create_file( dbg_workspace_root ) )
    {
        if ( std::fstream project_root( dbg_workspace_root ); project_root )
        {
            if ( exists( manytypes_root / "global.h" ) )
                project_root << "#include \"../global.h\"";
        }
        // maybe error handle if the creation fails
    }

    std::queue<std::filesystem::path> paths_to_check;
    paths_to_check.push( manytypes_root );

    bool must_refresh = false;
    for ( auto& dir_item : std::filesystem::directory_iterator( dbg_workspace ) )
    {
        if ( dir_item.is_regular_file() )
        {
            const auto& file = dir_item.path();
            if ( file.extension() != ".h" && file.extension() != ".hpp" )
                continue;

            auto last_write = last_write_time( file );
            if ( last_save_time[file.filename().string()] != last_write )
                must_refresh = true;

            last_save_time[file.filename().string()] = last_write;
        }
    }

    if ( !must_refresh )
        return;

    const std::filesystem::path global = manytypes_root / "global.h";
    create_file( global );

    const std::filesystem::path src_root = manytypes_artifacts / "source.cpp";
    create_file( src_root );

    // write dummy include
    bool abort_parse = false;
    {
        std::ofstream src_file( src_root, std::ios::trunc );
        if ( src_file )
            src_file << "#include \"../" << norm_image_name << "/project.h\"";
        else
            abort_parse = true;
    }

    if ( abort_parse )
    {
        dprintf( "aborting header parse, unable to write %s\n", src_root.string().c_str() );
        return;
    }

    try
    {
        BOOL is_bit32;
        IsWow64Process( DbgGetProcessHandle(), &is_bit32 );

        std::lock_guard typedb_lock( g_typedb_mutex );

        auto typedb = mt::parse_root_source( src_root, is_bit32 );
        if ( typedb )
        {
            auto& db = *typedb;
            g_typedb = typedb;

            auto target_db = manytypes_artifacts / ( norm_image_name + ".json" );
            if ( std::ofstream json_db( target_db, std::ios::trunc ); json_db )
            {
                json_db << create_x64dbg_database( db );
                json_db.close();

                auto types_path = relative( target_db, std::filesystem::current_path() ).string();
                DbgCmdExec( std::format( "ClearTypes \"{}\"", types_path ).c_str() );
                DbgCmdExec( std::format( "LoadTypes \"{}\"", types_path ).c_str() );

                dprintf( "updated json db %s\n", types_path.c_str() );
            }
            else
            {
                dprintf( "failed to update json db %s\n", target_db.string().c_str() );
            }
        }
        else
        {
            dprintf( "unable to parse source tree\n" );
        }
    }
    catch ( mt::Exception& e )
    {
        dprintf( "exception occurred while parsing header %s\n", e.what() );
    }
}

void set_workspace_target( const char* image_name )
{
    g_curr_image_name = image_name;
}

void plugin_menu_select( const int entry )
{
    switch ( entry )
    {
    case 0:
        const auto manytypes_root = std::filesystem::current_path() / "ManyTypes";
        const auto target_path_cstr = reinterpret_cast<LPCSTR>( manytypes_root.u8string().c_str() );

        ShellExecuteA( nullptr, "open", "explorer.exe", target_path_cstr, nullptr, SW_SHOWNORMAL );
        break;
    }
}

#include <functional>

bool plugin_handle_pt( int argc, char** t )
{
    if ( argc < 2 )
    {
        dputs( "Usage: pt typename" );
        return false;
    }

    std::lock_guard typedb_lock( g_typedb_mutex );

    auto& typedb = *g_typedb;
    auto types = typedb.get_types();

    bool success = false;
    for ( auto& [id, data] : types )
    {
        try
        {
            bool found_type = false;
            try
            {
                auto type_name = typedb.get_type_print( id );
                if ( type_name == t[1] )
                {
                    found_type = true;
                }
            }
            catch ( ... )
            {
            }

            if ( found_type )
            {
                mt::formatter_clang fmt( typedb, true, "    " );
                dprintf( "\n%s\n", fmt.print_type( id ).c_str() );

                success = true;
            }
        }
        catch ( ... )
        {
            dprintf( "err: exception while printing type " );
        }

        if ( success )
            break;
    }

    return success;
}

// Initialize your plugin data here.
bool plugin_init( PLUG_INITSTRUCT* initStruct )
{
    return true;
}

// Deinitialize your plugin data here.
void plugin_stop()
{
}

// Do GUI/Menu related things here.
void plugin_setup()
{
    _plugin_menuaddentry( hMenu, OPEN_EXPLORER_MANYTYPES, "Open in Explorer" );
    _plugin_registercommand( pluginHandle, "pt", plugin_handle_pt, true );

    //_plugin_menuaddentry(hMenu, OPEN_VSCODE_MANYTYPES, "Open ManyTypes VSCode");
}