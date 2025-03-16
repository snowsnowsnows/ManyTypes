#pragma once

#include <utility>
#include <vector>

#include "models/field.h"
#include "manytypes-lib/types/models/named_sized.h"

enum class call_conv
{
    unk = 0 ,
    cdecl ,
    stdcall ,
    thiscall ,
    fastcall ,
};

class function_t final : public named_sized_type_t
{
public:
    function_t( std::string name,
                const call_conv conv,
                const type_id return_type,
                const std::vector<type_id>& args
    )
        : name( std::move( name ) ), conv( conv ), return_type( return_type ), args( args )
    {
    }

    std::string name_of( ) override
    {
        return name;
    }

    size_t size_of( type_size_resolver& tr ) override
    {
        return 0;
    }

    const std::vector<type_id>& get_args( )
    {
        return args;
    }

    [[nodiscard]] type_id get_return_type( ) const
    {
        return return_type;
    }

private:
    std::string name;
    call_conv conv;
    type_id return_type;
    std::vector<type_id> args;
};
