#pragma once
#include <utility>

#include "manytypes-lib/types/models/named_sized.h"

namespace mt
{
// todo: combine these into one
class typedef_type_t final : public dependent_t
{
public:
    typedef_type_t( std::string alias, const type_id type )
        : alias( std::move( alias ) ), type( type )
    {
    }

    std::vector<type_id> get_dependencies() const override
    {
        return { type };
    }

    std::string alias;
    type_id type;
};

// todo: this is a misleading name
// this is only used when theres a type such as `struct some_struct`
// and it forwards this type to some_struct
class elaborated_t final : public dependent_t
{
public:
    explicit elaborated_t( const type_id type )
        : type( type )
    {
    }

    explicit elaborated_t( const type_id type, std::string sugar, std::string scope, std::string type_name )
        : type( type ), sugar( std::move( sugar ) ), scope( std::move( scope ) ), type_name( std::move( type_name ) )
    {
    }

    std::vector<type_id> get_dependencies() const override
    {
        return { type };
    }

    type_id get_forward() const
    {
        return type;
    }

    bool is_clear()
    {
        return sugar.empty() && scope.empty();
    }

    type_id type;

    std::string sugar;
    std::string scope;
    std::string type_name;
};
} // namespace mt
