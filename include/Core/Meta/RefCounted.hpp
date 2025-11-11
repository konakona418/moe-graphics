#pragma once

#include "Core/Common.hpp"
#include "Core/Meta/TypeTraits.hpp"

MOE_BEGIN_NAMESPACE

namespace Meta {
    template<typename T>
    struct IsRefCounted {
        static auto test(int)
                -> decltype(Meta::DeclareValue<T>().retain(),
                            Meta::DeclareValue<T>().release(),
                            Meta::TrueType{});

        static auto test(...) -> Meta::FalseType;

        static constexpr bool value = decltype(test(0))::value;
    };
}// namespace Meta

MOE_END_NAMESPACE
