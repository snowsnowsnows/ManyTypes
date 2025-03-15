#pragma once
#include <filesystem>

#include <cassert>

#include "clang-c/Index.h"

#include <filesystem>
#include <format>
#include <fstream>

#include "manytypes-lib/database.h"
#include "manytypes-lib/clang-database.h"
#include "manytypes-lib/struct/enum.h"

void parse_header( std::filesystem::path& header_path )
{
    const std::filesystem::path current_path = std::filesystem::current_path( );
    const std::filesystem::path stub_source = current_path / "stub_include.cpp";

    std::ofstream include_stub( stub_source, std::ios::out | std::ios::trunc );
    include_stub << header_data;

    include_stub.close( );

    std::vector<std::string> clang_args =
    {
        // "-I" + std::string(argv[1]),
        "-x",
        "c++",
        "-fms-extensions",
        "-Xclang",
        "-ast-dump",
        "-fsyntax-only",
        "-fms-extensions",
    };

    std::vector<const char*> c_args;
    for ( const auto& arg : clang_args )
        c_args.push_back( arg.c_str( ) );

    if ( const auto index = clang_createIndex( 0, 1 ) )
    {
        CXTranslationUnit tu = nullptr;
        const auto error = clang_parseTranslationUnit2(
            index,
            stub_source.string( ).c_str( ),
            c_args.data( ),
            static_cast<int>(c_args.size( )),
            nullptr,
            0,
            CXTranslationUnit_DetailedPreprocessingRecord |
            CXTranslationUnit_PrecompiledPreamble |
            CXTranslationUnit_SkipFunctionBodies |
            CXTranslationUnit_ForSerialization,
            &tu );

        if ( error == CXError_Success )
        {
            const CXCursor cursor = clang_getTranslationUnitCursor( tu );
            clang_visitChildren( cursor, visit_cursor, nullptr );
        }
        else
        {
            printf( "CXError: %d\n", error );
            return EXIT_FAILURE;
        }

        clang_disposeTranslationUnit( tu );
    }
}
