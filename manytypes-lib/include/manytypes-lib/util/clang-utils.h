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
        const std::size_t hash1 = std::hash<int>{ }( t.kind );
        const std::size_t hash2 = std::hash<void*>{ }( t.data[ 0 ] );
        const std::size_t hash3 = std::hash<void*>{ }( t.data[ 1 ] );

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

template < typename... Args >
class clang_spelling_str
{
public:
    explicit clang_spelling_str( Args... args )
        : clang_string( clang_getCursorSpelling( args... ) ),
          str_view( clang_getCString( clang_string ) )
    {
    }

    [[nodiscard]] std::string_view get( ) const
    {
        return str_view;
    }

    explicit operator std::string_view( ) const
    {
        return str_view;
    }

    operator std::string( ) const
    {
        return std::string( str_view );
    }

    ~clang_spelling_str( )
    {
        clang_disposeString( clang_string );
    }

private:
    CXString clang_string;
    std::string_view str_view;
};

inline std::optional<std::string> get_type_name( const CXType& type, const uint32_t pointer_level = 0 )
{
    auto current_type = clang_getUnqualifiedType( type );
    if ( current_type.kind == CXType_ConstantArray ||
        current_type.kind == CXType_IncompleteArray ||
        current_type.kind == CXType_VariableArray )
    {
        current_type = clang_getElementType( current_type );
    }
    else if ( current_type.kind == CXType_Elaborated )
    {
        current_type = clang_Type_getNamedType( current_type );
    }

    if ( current_type.kind == CXType_Pointer )
    {
        const CXType next_type = clang_getPointeeType( current_type );
        if ( next_type.kind != CXType_Invalid )
            return get_type_name( next_type, pointer_level + 1 );
    }

    const CXCursor decl_cursor = clang_getTypeDeclaration( current_type );
    if ( clang_Cursor_isAnonymous( decl_cursor ) )
        return std::nullopt;

    const auto out_spelling = try_get_anon_name( current_type );
    if ( !out_spelling )
    {
        const auto spelling = clang_getTypeSpelling( current_type );
        std::string result = std::string( clang_getCString( spelling ) ) +
            std::string(
                pointer_level, '*' );

        clang_disposeString( spelling );
        return result;
    }

    return std::string( out_spelling ) + std::string( pointer_level, '*' );
};

inline std::pair<std::vector<size_t>, CXType> get_array_dimensions( CXType type )
{
    std::vector<size_t> dimensions;

    while ( type.kind == CXType_ConstantArray )
    {
        const long long size = clang_getArraySize( type );
        dimensions.push_back( static_cast<int>(size) );

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
