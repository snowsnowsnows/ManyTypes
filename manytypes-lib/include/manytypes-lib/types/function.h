#pragma once

#include <utility>
#include <vector>

#include "models/field.h"
#include "manytypes-lib/types/models/named_sized.h"

enum class call_conv
{
    unk,
    cc_cdecl,
    cc_stdcall,
    cc_thiscall,
    cc_fastcall,
};

class function_t final : public dependent_t
{
public:
    function_t(
        const call_conv conv,
        const type_id return_type,
        const std::vector<type_id>& args )
        : conv( conv ), return_type( return_type ), args( args )
    {
    }

    call_conv get_call_conv() const
    {
        return conv;
    }

    const std::vector<type_id>& get_args() const
    {
        return args;
    }

    [[nodiscard]] type_id get_return_type() const
    {
        return return_type;
    }

    std::vector<type_id> get_dependencies() override
    {
        return std::vector( args.begin(), args.end() );
    }

private:
    call_conv conv;
    type_id return_type;
    std::vector<type_id> args;
};
