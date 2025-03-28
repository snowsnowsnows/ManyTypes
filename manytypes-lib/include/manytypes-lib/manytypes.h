#pragma once
#include <filesystem>
#include <cassert>
#include <filesystem>
#include <format>
#include <fstream>

#include <clang-c/Index.h>
#include "manytypes-lib/util/clang-context.h"

namespace mt
{
std::optional<type_database_t> parse_root_source( const std::filesystem::path& src_path, bool bit32 = false );
std::string create_header( const type_database_t& db );
std::string create_x64dbg_database( const type_database_t& db );
} // namespace mt
