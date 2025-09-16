#pragma once

#include "Core/Common.hpp"

// common resource cache

namespace moe {
    template<typename ResT>
    using SharedResource = SharedPtr<ResT>;

    template<
            typename ResIdT,
            typename ResT>
    using ResHashMap = UnorderedMap<ResIdT, SharedResource<ResT>>;

    template<
            typename ResIdT,
            typename ResT,
            typename LoaderFunctorT>
    struct VulkanCache {
    public:
        Optional<SharedResource<ResT>> get(ResIdT id) {
            auto it = resources.find(id);
            if (it != resources.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        template<typename... Args>
        using EnableIfInvocable = std::enable_if_t<
                std::is_invocable_v<
                        LoaderFunctorT, Args...>>;

        template<typename... Args, typename = EnableIfInvocable<Args...>>
        SharedResource<ResT> load(Args&&... args) {
            ResIdT id;
            if (!recycledIds.empty()) {
                id = recycledIds.front();
                recycledIds.pop_front();
            } else {
                id = idCounter++;
            }

            return resources.emplace(id, LoaderFunctorT{}(std::forward<Args>(args)...)).first->second;
        }

        void leak(ResIdT id) {
            resources.erase(id);
            recycledIds.push_back(id);
        }

    private:
        ResIdT idCounter{0};
        Deque<ResIdT> recycledIds;
        ResHashMap<ResIdT, ResT> resources;
    };
}// namespace moe