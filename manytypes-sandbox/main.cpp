#include "manytypes-lib/manytypes.h"
#include <iostream>
#include <filesystem>
#include <fstream>

int main( int argc, char* argv[] )
{
    if ( argc < 3 )
    {
        std::cout << "Usage: " << argv[0] << " <header-file-path> <json-output-path>" << std::endl;
        return 1;
    }

    std::filesystem::path header_path = argv[1];
    std::filesystem::path out_json = argv[2];
    std::filesystem::path source = header_path.parent_path() / "source.cpp";

    {
        std::ofstream f( source, std::ios::trunc );
        f << "#include \"" << header_path.filename().string() << "\"";
    }

    try
    {
        auto db = mt::parse_root_source( source );

        std::ofstream out_db( out_json );
        out_db << mt::create_x64dbg_database( *db );

        std::cout << "generated: " << out_json << std::endl;
    }
    catch ( std::exception& exc )
    {
        std::cout << exc.what() << std::endl;
        return 1;
    }

    return 0;
}
