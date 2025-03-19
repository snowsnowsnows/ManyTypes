#pragma once
#include <optional>
#include <string>
#include <vector>
#include <clang-c/Index.h>

#include <clang-c/Index.h>
#include <string_view>

struct cx_type_hash
{
    std::size_t operator()( const CXType& t ) const noexcept
    {
        const std::size_t hash1 = std::hash<int>{}( t.kind );
        const std::size_t hash2 = std::hash<void*>{}( t.data[0] );
        const std::size_t hash3 = std::hash<void*>{}( t.data[1] );

        return hash1 ^ hash2 << 1 ^ hash3 << 2;
    }
};

struct cx_type_equal
{
    bool operator()( const CXType& lhs, const CXType& rhs ) const
    {
        return clang_equalTypes( lhs, rhs );
    }
};

enum spelling_settings
{
    UNQUALIFIED = 1,
};

template<typename T>
class clang_spelling_str
{
public:
    explicit clang_spelling_str( T arg )
        : inited( false )
    {
        if ( !clang_Cursor_isAnonymous( arg ) )
        {
            clang_string = clang_getCursorSpelling( arg );
            str_view = clang_getCString( clang_string );

            inited = true;
        }
    }

    [[nodiscard]] std::string_view get() const
    {
        return str_view;
    }

    explicit operator std::string_view() const
    {
        return str_view;
    }

    operator std::string() const
    {
        return std::string( str_view );
    }

    ~clang_spelling_str()
    {
        if ( inited )
            clang_disposeString( clang_string );
    }

private:
    CXString clang_string;
    std::string_view str_view;

    bool inited;
};

inline std::pair<std::vector<size_t>, CXType> get_array_dimensions( CXType type )
{
    std::vector<size_t> dimensions;

    while ( type.kind == CXType_ConstantArray )
    {
        const long long size = clang_getArraySize( type );
        dimensions.push_back( static_cast<int>( size ) );

        type = clang_getArrayElementType( type );
    }

    return { dimensions, type };
}

inline std::pair<size_t, CXType> get_pointer_level( CXType type )
{
    size_t pointer_count = 0;
    while ( type.kind == CXType_Pointer )
    {
        pointer_count++;
        type = clang_getPointeeType( type );
    }

    return { pointer_count, type };
}
