#include "manytypes-lib/manytypes.h"
#include <iostream>

int main()
{
    const auto source = std::filesystem::current_path() / "source.cpp";
    {
        std::ofstream f( source, std::ios::trunc );
        f << "#include \"phnt.h\"";
    }

    auto db = parse_root_source( source );
    std::cout << create_header( *db ) << std::endl;
}
