#include "plugin.h"

#include <filesystem>
#include <queue>
#include <string>

#include "manytypes-lib/manytypes.h"

std::atomic<const char*> curr_image_name;
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
    }

    return true;
}

void plugin_run_loop()
{
    const char* curr_image = curr_image_name;
    if ( curr_image == nullptr )
        return;

    std::filesystem::path image_path( curr_image );

    std::string norm_image_name = image_path.stem().string();
    std::ranges::replace( norm_image_name, ' ', '-' );

    const auto run_root = std::filesystem::current_path();

    const auto manytypes_root = run_root / "ManyTypes";
    const auto dbg_workspace = manytypes_root / norm_image_name;
    const auto dbg_workspace_root = dbg_workspace / "project.h";
    create_file( dbg_workspace_root );

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

    if ( must_refresh )
    {
        const std::filesystem::path global = manytypes_root / "global.h";
        create_file( global );

        const std::filesystem::path src_root = manytypes_root / "source.cpp";
        create_file( src_root );

        // write dummy include
        bool abort_parse = false;
        {
            std::ofstream src_file( src_root, std::ios::trunc );
            if ( src_file )
                src_file << "#include \"" << norm_image_name << "/project.h\"";
            else
                abort_parse = true;
        }

        if ( !abort_parse )
        {
            try
            {
                auto opt_db = mt::parse_root_source( src_root );
                if ( opt_db )
                {
                    auto& db = *opt_db;

                    auto target_db = dbg_workspace / ( norm_image_name + ".json" );
                    if ( std::ofstream json_db( target_db, std::ios::trunc ); json_db )
                    {
                        json_db << mt::create_x64dbg_database( db );
                        json_db.close();

                        auto types_path = relative( target_db, std::filesystem::current_path() ).string();
                        DbgCmdExec( std::format( "ClearTypes \"{}\"", types_path.c_str() ).c_str() );
                        DbgCmdExec( std::format( "LoadTypes \"{}\"", types_path.c_str() ).c_str() );

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
        else
        {
            dprintf( "aborting header parse, unable to write %s\n", src_root.c_str() );
        }
    }
}

void set_workspace_target( const char* image_name )
{
    curr_image_name = image_name;
}

// Initialize your plugin data here.
bool plugin_init( PLUG_INITSTRUCT* initStruct )
{
    dprintf( "plugin_init(pluginHandle: %d)\n", pluginHandle );
    return true;
}

// Deinitialize your plugin data here.
void plugin_stop()
{
    dprintf( "plugin_stop(pluginHandle: %d)\n", pluginHandle );
}

// Do GUI/Menu related things here.
void plugin_setup()
{
    dprintf( "plugin_setup(pluginHandle: %d)\n", pluginHandle );
}