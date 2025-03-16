#include "manytypes-lib/manytypes.h"

int main( )
{
    const auto current = std::filesystem::current_path( );
    const auto source = current / "source.cpp";
    create_directories( source );

    std::fstream f( source, std::ios::trunc );
    f << "#include \"phnt.h\"";

    auto db = parse_root_source( source );
}
