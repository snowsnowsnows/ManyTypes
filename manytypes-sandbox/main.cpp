#include "manytypes-lib/manytypes.h"

int main( )
{
    const auto source = std::filesystem::current_path( ) / "source.cpp";
    {
        std::ofstream f( source, std::ios::trunc );
        f << "#include \"phnt.h\"";
    }

    auto db = parse_root_source( source );
}
