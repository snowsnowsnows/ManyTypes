#pragma once

template< class... Ts >
struct overloads : Ts...
{
    using Ts::operator()...;
};