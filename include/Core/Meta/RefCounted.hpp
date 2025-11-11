#pragma once

#include "Core/Common.hpp"

MOE_BEGIN_NAMESPACE

namespace Meta {
    template<typename T>
    struct IsRefCounted {
        static auto test(int) -> decltype(std::declval<T>().retain(), std::declval<T>().release(), std::true_type{});
        static auto test(...) -> std::false_type;

        static constexpr bool value = decltype(test(0))::value;
    };
}// namespace Meta

MOE_END_NAMESPACE
