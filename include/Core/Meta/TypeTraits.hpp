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

    template<bool B, typename T = void>
    using EnableIfT = typename EnableIf<B, T>::type;

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

    template<typename T>
    struct IsCompleteType {
        template<typename U>
        static auto test(int) -> decltype(sizeof(DeclareValue<U>()), TrueType{});

        template<typename U>
        static auto test(...) -> FalseType;

        static constexpr bool value = decltype(test<T>(0))::value;
    };

    template<typename T>
    constexpr bool IsCompleteTypeV = IsCompleteType<T>::value;
}// namespace Meta

MOE_END_NAMESPACE
