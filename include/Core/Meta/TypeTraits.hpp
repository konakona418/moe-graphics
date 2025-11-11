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

    template<typename T>
    struct AddRValueRef {
        using type = T&&;
    };

    template<typename T>
    struct AddRValueRef<T&> {
        using type = T&&;
    };

    template<typename T>
    struct AddRValueRef<T&&> {
        using type = T&&;
    };

    template<typename T>
    using AddRValueRefT = typename AddRValueRef<T>::type;

    template<typename T>
    AddRValueRefT<T> DeclareValue() noexcept;

    template<typename T = int, T v = T{}>
    struct IntegralConstant {
        static constexpr T value = v;
    };

    using TrueType = IntegralConstant<bool, true>;
    using FalseType = IntegralConstant<bool, false>;
}// namespace Meta

MOE_END_NAMESPACE
