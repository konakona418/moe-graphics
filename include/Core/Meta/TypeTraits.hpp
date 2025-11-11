#pragma once

#include "Core/Common.hpp"

MOE_BEGIN_NAMESPACE

namespace Meta {
    template<bool B, typename T = void>
    struct EnableIf {
        using type = T;
    };

    template<typename T>
    struct EnableIf<false, T> {};

    template<typename T>
    using EnableIfT = typename EnableIf<T::value, T>::type;
}// namespace Meta

MOE_END_NAMESPACE
