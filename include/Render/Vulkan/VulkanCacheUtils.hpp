#pragma once

#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    template<typename IdT>
    struct VulkanCacheIdAllocator {
    public:
        VulkanCacheIdAllocator() = default;
        ~VulkanCacheIdAllocator() = default;

        IdT allocateId() {
            if (!m_recycledIds.empty()) {
                auto id = m_recycledIds.front();

                m_recycledIds.pop_front();
                m_recycledIdLookup.erase(id);

                return id;
            }
            return m_nextId++;
        }

        void recycleId(IdT id) {
            m_recycledIds.push_back(id);
            m_recycledIdLookup.insert(id);
        }

        bool isIdValid(IdT id) const {
            return m_recycledIdLookup.find(id) == m_recycledIdLookup.end() &&
                   id < m_nextId;
        }

        void reset() {
            m_nextId = 0;
            m_recycledIds.clear();
            m_recycledIdLookup.clear();
        }

    private:
        IdT m_nextId{0};

        Deque<IdT> m_recycledIds;
        UnorderedSet<IdT> m_recycledIdLookup;
    };
}// namespace moe