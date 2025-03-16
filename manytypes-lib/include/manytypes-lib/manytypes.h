#pragma once
#include <filesystem>
#include <cassert>
#include <filesystem>
#include <format>
#include <fstream>

#include <clang-c/Index.h>
#include "manytypes-lib/util/clang-context.h"

inline std::optional<type_database_t> parse_root_source( const std::filesystem::path& src_path );
inline std::vector<type_id> order_database_nodes( const type_database_t& db );