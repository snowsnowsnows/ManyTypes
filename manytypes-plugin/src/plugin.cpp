#include "plugin.h"

#include <filesystem>
#include <queue>
#include <string>
#include <mutex>

#include "manytypes-lib/manytypes.h"
#include "manytypes-lib/formatter/clang.h"
#include "manytypes-lib/util/util.h"

static std::string g_curr_image_name;

static std::mutex g_typedb_mutex;
static std::optional<mt::type_database_t> g_typedb = std::nullopt;
static std::atomic_bool g_loop_stop;
static std::thread g_loop_thread;

static std::unordered_map<std::u8string, std::filesystem::file_time_type> last_save_time;

static bool create_file( const std::filesystem::path& path, const bool hidden = false )
{
    if ( !exists( path ) )
    {
        create_directories( path.parent_path() );
        std::ofstream file( path );

        return true;
    }

    return false;
}

static void plugin_run_loop()
{
    if ( g_curr_image_name.empty() )
        return;

    auto image_path = std::filesystem::u8path( g_curr_image_name );

    auto norm_image_name = image_path.stem();
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

            auto file_string = file.filename().u8string();
            auto last_write = last_write_time( file );
            if ( last_save_time[file_string] != last_write )
                must_refresh = true;

            last_save_time[file_string] = last_write;
        }
    }

    if ( !must_refresh )
        return;

    const std::filesystem::path global = manytypes_root / "global.h";
    create_file( global );

    const std::filesystem::path src_root = manytypes_artifacts / "source.cpp";
    create_file( src_root );

    try
    {
#ifdef _M_IX86
        BOOL is_bit32 = true;
#else
        BOOL is_bit32 = false;
#endif
        std::lock_guard typedb_lock( g_typedb_mutex );

        auto typedb = mt::parse_root_source( src_root, is_bit32 );
        if ( typedb )
        {
            auto& db = *typedb;
            g_typedb = typedb;

            std::filesystem::path norm_image_json = norm_image_name;
            norm_image_json.replace_extension( ".json" );

            std::filesystem::path target_db = manytypes_artifacts / norm_image_json;
            if ( std::ofstream json_db( target_db, std::ios::trunc ); json_db )
            {
                json_db << create_x64dbg_database( db );
                json_db.close();

                std::u8string types_path = relative( target_db, std::filesystem::current_path() ).u8string();
                DbgCmdExec( std::format( "ClearTypes \"{}\"", (char*)types_path.c_str() ).c_str() );
                DbgCmdExec( std::format( "LoadTypes \"{}\"", (char*)types_path.c_str() ).c_str() );

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

void set_workspace_target( std::string image_name )
{
    g_curr_image_name = std::string( image_name );
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
    // HACK: there is a bug in libclang.dll that causes a crash if unloaded before
    // process exit. As a workaround we increase the ref count of the library, so
    // that it doesn't get unloaded until the process exits. This happens because
    // FlsFree is not called for every FlsAlloc instance.
    // Reference: https://github.com/llvm/llvm-project/issues/154361
    LoadLibraryW( L"libclang.dll" );

    g_loop_thread = std::thread( []
        {
            while ( !g_loop_stop )
            {
                std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
                plugin_run_loop();
            }
            // do not remove this comment (clang-format is retarded)
        } );
    return true;
}

// Deinitialize your plugin data here.
void plugin_stop()
{
    g_loop_stop = true;
    std::lock_guard typedb_lock( g_typedb_mutex );
    g_loop_thread.join();
}

// Do GUI/Menu related things here.
void plugin_setup()
{
    _plugin_menuaddentry( hMenu, OPEN_EXPLORER_MANYTYPES, "Open in Explorer" );
    _plugin_registercommand( pluginHandle, "pt", plugin_handle_pt, true );

    //_plugin_menuaddentry(hMenu, OPEN_VSCODE_MANYTYPES, "Open ManyTypes VSCode");
}