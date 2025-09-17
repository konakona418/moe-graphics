#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <tcb/span.hpp>

namespace moe {
    template<typename... TArgs>
    using UniquePtr = std::unique_ptr<TArgs...>;

    template<typename... TArgs>
    using SharedPtr = std::shared_ptr<TArgs...>;

    using String = std::string;

    using StringView = std::string_view;

    template<typename T, size_t N>
    using Array = std::array<T, N>;

    template<typename... TArgs>
    using Vector = std::vector<TArgs...>;

    template<typename... TArgs>
    using Queue = std::queue<TArgs...>;

    template<typename... TArgs>
    using Deque = std::deque<TArgs...>;

    template<typename T>
    using Optional = std::optional<T>;

    template<typename... TArgs>
    using Function = std::function<TArgs...>;

    template<typename T>
    using Span = tcb::span<T>;

    template<typename... TArgs>
    using Variant = std::variant<TArgs...>;

    template<typename... TArgs>
    using UnorderedMap = std::unordered_map<TArgs...>;

    template<typename... TArgs>
    using UnorderedSet = std::unordered_set<TArgs...>;
}// namespace moe