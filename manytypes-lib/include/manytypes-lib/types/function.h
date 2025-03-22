#pragma once

#include <utility>
#include <vector>

#include "models/field.h"
#include "manytypes-lib/types/models/named_sized.h"

enum class call_conv
{
    unk ,
    cc_cdecl ,
    cc_stdcall ,
    cc_thiscall ,
    cc_fastcall ,
};

class function_t final : public named_sized_type_t
{
public:
    function_t(
        const call_conv conv,
        const type_id return_type,
        const std::vector<type_id>& args
    )
        : conv( conv ), return_type( return_type ), args( args )
    {
    }

    std::string name_of( ) const override
    {
        return "";
    }

    size_t size_of( type_size_resolver& tr ) const override
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

    call_conv conv;
    type_id return_type;
    std::vector<type_id> args;
};
