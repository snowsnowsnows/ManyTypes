#include "plugin.h"

#include <filesystem>
#include <queue>
#include <string>
#include <mutex>

#include "manytypes-lib/manytypes.h"
#include "manytypes-lib/formatter/clang.h"
#include "manytypes-lib/util/util.h"

static std::u8string g_curr_image_name;

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

    const auto run_root = std::filesystem::current_path();

    const auto manytypes_root = run_root / "ManyTypes";
    const auto manytypes_artifacts = manytypes_root / ".artifacts";

    create_directories( manytypes_artifacts );

    const auto dbg_workspace = manytypes_root / g_curr_image_name;
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

    try
    {
#ifdef _M_IX86
        BOOL is_bit32 = true;
#else
        BOOL is_bit32 = false;
#endif
        std::lock_guard typedb_lock( g_typedb_mutex );

        auto typedb = mt::parse_root_source( dbg_workspace_root, is_bit32 );
        if ( typedb )
        {
            auto& db = *typedb;
            g_typedb = typedb;

            std::filesystem::path norm_image_json = g_curr_image_name;
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
    auto image_path = std::filesystem::path( (char8_t*)image_name.c_str() );
    auto norm_image_name = image_path.stem();

    g_curr_image_name = norm_image_name.u8string();
}

static std::wstring utf8_to_utf16( const char* str )
{
    int required_size = MultiByteToWideChar( CP_UTF8, 0, str, -1, 0, 0 );
    if ( required_size <= 0 )
        return {};

    std::wstring utf16( required_size - 1, L'\0' );
    MultiByteToWideChar( CP_UTF8, 0, str, -1, utf16.data(), required_size );
    return utf16;
}

void plugin_menu_select( const int entry )
{
    if ( g_curr_image_name.empty() )
        return;

    const auto manytypes_root = std::filesystem::current_path() / "ManyTypes" / g_curr_image_name;
    switch ( entry )
    {
    case EDIT_MANYTYPES:
    {
        const auto manytypes_project = manytypes_root / "project.h";
        if (!exists( manytypes_project ))
        {
            dprintf("project.h file does not exist. start a debug session\n");
            break;
        }

        const auto target_path = utf8_to_utf16( (char*)manytypes_project.u8string().c_str() );
        ShellExecuteW( nullptr, L"open", target_path.c_str(), nullptr, nullptr, SW_SHOWNORMAL );
    }
    case OPEN_EXPLORER_MANYTYPES:
    {
        if (!exists( manytypes_root ))
        {
            dprintf("project root does not exist. start a debug session\n");
            break;
        }

        const auto target_path = utf8_to_utf16( (char*)manytypes_root.u8string().c_str() );
        ShellExecuteW( nullptr, L"explore", target_path.c_str(), nullptr, nullptr, SW_SHOWNORMAL );
    }
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
    _plugin_menuaddentry( hMenu, OPEN_EXPLORER_MANYTYPES, "Open Project Folder" );
    _plugin_menuaddentry( hMenu, EDIT_MANYTYPES, "Edit Project Header" );
    _plugin_registercommand( pluginHandle, "pt", plugin_handle_pt, true );

    //_plugin_menuaddentry(hMenu, OPEN_VSCODE_MANYTYPES, "Open ManyTypes VSCode");
}