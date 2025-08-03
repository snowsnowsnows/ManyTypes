#include "manytypes-lib/manytypes.h"
#include <iostream>
#include <filesystem>
#include <fstream>

int main( int argc, char* argv[] )
{
    if ( argc < 4 )
    {
        std::cout << "Usage: " << argv[0] << " <header-file-path> <json-output-path> <isbit32>" << std::endl;
        return 2;
    }

    std::filesystem::path header_path = argv[1];
    std::filesystem::path out_json = argv[2];

    bool isbit32 = std::string( argv[3] ) == "1";

    try
    {
        auto db = mt::parse_root_source( header_path, isbit32 );

        std::ofstream out_db( out_json );
        out_db << mt::create_x64dbg_database( *db );

        std::cout << "generated: " << out_json << std::endl;
    }
    catch ( std::exception& exc )
    {
        std::cout << "error: " << exc.what() << std::endl;
        return 1;
    }

    return 0;
}
