#pragma once

#include <array>
#include <cassert>
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

    template<typename FirstT, typename SecondT>
    using Pair = std::pair<FirstT, SecondT>;
}// namespace moe

#include "Core/Logger.hpp"

#define MOE_ASSERT(_cond, _msg) (assert(_cond&& _msg))

#define MOE_LOG_AND_THROW(_msg) \
    Logger::critical(_msg);     \
    throw std::runtime_error(_msg);

#define MOE_VK_CHECK_MSG(expr, msg)                     \
    do {                                                \
        VkResult result = expr;                         \
        if (result != VK_SUCCESS) {                     \
            Logger::critical(msg);                      \
            throw std::runtime_error(std::string(msg)); \
        }                                               \
    } while (false)
