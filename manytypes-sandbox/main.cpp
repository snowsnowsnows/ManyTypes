#include "manytypes-lib/manytypes.h"
#include <iostream>

int main()
{
    const auto source = std::filesystem::current_path() / "source.cpp";
    {
        std::ofstream f( source, std::ios::trunc );
        f << "#include \"phnt.h\"";
    }

    std::ofstream out_header("out_header.h");

    auto db = parse_root_source( source );
    out_header << create_header( *db );
}
