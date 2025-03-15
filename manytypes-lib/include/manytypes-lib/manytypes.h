#pragma once
#include <filesystem>
#include <cassert>
#include <filesystem>
#include <format>
#include <fstream>

#include <clang-c/Index.h>
#include "manytypes-lib/util/clang-context.h"

inline std::optional<type_database_t> parse_header( std::filesystem::path& header_path );