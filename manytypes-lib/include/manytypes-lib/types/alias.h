#pragma once
#include "manytypes-lib/types/models/named_sized.h"

// todo: combine these into one
class typedef_type_t final : public dependent_t
{
public:
    typedef_type_t( std::string alias, const type_id type )
        : alias( std::move( alias ) ), type( type )
    {
    }

    std::vector<type_id> get_dependencies() override
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
    explicit elaborated_t( const type_id type, const std::string& elaborated_name )
        : type( type ), elaborated_name(elaborated_name)
    {
    }

    std::vector<type_id> get_dependencies() override
    {
        return { type };
    }

    type_id get_forward() const
    {
        return type;
    }

    type_id type;
    std::string elaborated_name;
};
