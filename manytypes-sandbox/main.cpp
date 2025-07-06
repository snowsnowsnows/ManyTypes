#include "manytypes-lib/manytypes.h"
#include <iostream>

int main()
{
    const auto source = std::filesystem::current_path() / "source.cpp";
    {
        std::ofstream f( source, std::ios::trunc );
        f << "#include \"phnt.h\"";
    }

    std::ofstream out_header( "out_header.h" );

    try
    {
        auto db = mt::parse_root_source( source );
        out_header << mt::create_header( *db );

        std::ofstream out_db( "out_db.json" );
        out_db << mt::create_x64dbg_database( *db );
    }
    catch ( std::exception& exc)
    {
        std::cout << exc.what() << std::endl;
    }
}
