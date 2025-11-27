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

    template<typename T>
    struct RemoveReference {
        using type = T;
    };

    template<typename T>
    struct RemoveReference<T&> {
        using type = T;
    };

    template<typename T>
    struct RemoveReference<T&&> {
        using type = T;
    };

    template<typename T>
    using RemoveReferenceT = typename RemoveReference<T>::type;

    template<typename T>
    struct RemoveQualifiers {
        using type = T;
    };

    template<typename T>
    struct RemoveQualifiers<const T> {
        using type = T;
    };

    template<typename T>
    struct RemoveQualifiers<volatile T> {
        using type = T;
    };

    template<typename T>
    struct RemoveQualifiers<const volatile T> {
        using type = T;
    };

    template<typename T>
    using RemoveQualifiersT = typename RemoveQualifiers<T>::type;

    template<typename T>
    struct IsVoid : FalseType {};

    template<>
    struct IsVoid<void> : TrueType {};

    template<typename T>
    constexpr bool IsVoidV = IsVoid<T>::value;

    template<typename T, typename U>
    struct IsSame : FalseType {};

    template<typename T>
    struct IsSame<T, T> : TrueType {};

    template<typename T, typename U>
    constexpr bool IsSameV = IsSame<T, U>::value;

    template<typename Fn, typename... Args>
    struct IsInvocable {
        struct NonInvocable {};

    private:
        template<typename F, typename... A>
        static auto test(int) -> decltype(DeclareValue<F>()(DeclareValue<A>()...));

        template<typename F, typename... A>
        static auto test(...) -> NonInvocable;

    public:
        static constexpr bool value = !IsSameV<decltype(test<Fn, Args...>(0)), NonInvocable>;
        using type = decltype(test<Fn, Args...>(0));
    };

    template<typename Fn, typename... Args>
    constexpr bool IsInvocableV = IsInvocable<Fn, Args...>::value;

    template<typename Fn, typename... Args>
    using IsInvocableT = typename IsInvocable<Fn, Args...>::type;

    template<typename T, typename... Args>
    struct IsConstructible {
    private:
        template<typename U, typename... A>
        static auto test(int) -> decltype(U(DeclareValue<A>()...), TrueType{});

        template<typename U, typename... A>
        static auto test(...) -> FalseType;

    public:
        static constexpr bool value = decltype(test<T, Args...>(0))::value;
    };

    template<typename T, typename... Args>
    constexpr bool IsConstructibleV = IsConstructible<T, Args...>::value;

    template<typename Base, typename Derived>
    struct IsBaseOf {
    private:
        static TrueType test(const Base*);
        static FalseType test(...);

    public:
        static constexpr bool value = decltype(test(static_cast<Derived*>(nullptr)))::value;
    };

    template<typename Base, typename Derived>
    constexpr bool IsBaseOfV = IsBaseOf<Base, Derived>::value;

    template<typename T>
    struct HasValueType {
    private:
        template<typename U>
        static auto test(int) -> decltype(typename U::value_type{}, TrueType{});

        template<typename U>
        static auto test(...) -> FalseType;

    public:
        static constexpr bool value = decltype(test<T>(0))::value;
    };

    template<typename T>
    constexpr bool HasValueTypeV = HasValueType<T>::value;

    template<typename T, typename... Args>
    struct InvokeResult {
    private:
        template<typename U, typename... A>
        static auto test(int) -> decltype(DeclareValue<U>()(DeclareValue<A>()...));

        template<typename U, typename... A>
        static auto test(...) -> void;

    public:
        using type = decltype(test<T, Args...>(0));
    };

    template<typename T, typename... Args>
    using InvokeResultT = typename InvokeResult<T, Args...>::type;

}// namespace Meta

MOE_END_NAMESPACE
