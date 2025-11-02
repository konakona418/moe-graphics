#pragma once

#include "Render/Vulkan/VulkanSwapBuffer.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


namespace moe {
    class VulkanEngine;
}

namespace moe {
    struct VulkanExpandableBuffer {
    public:
        static constexpr size_t DEFAULT_INITIAL_CAPACITY = 256;
        static constexpr size_t DEFAULT_EXPANSION_FACTOR = 2;

        VulkanExpandableBuffer() = default;
        ~VulkanExpandableBuffer() = default;

        void init(
                VulkanEngine& engine,
                VkBufferUsageFlags usage,
                size_t elementSize,
                size_t initialCapacity = DEFAULT_INITIAL_CAPACITY);

        void destroy();

        void pushBack(VkCommandBuffer commandBuffer, void* data, size_t elemCount = 1);

        template<typename T>
        void pushBack(VkCommandBuffer commandBuffer, const T& element) {
            pushBack(commandBuffer, &element, 1);
        }

        void clear() { m_elementCount = 0; }

    private:
        VulkanEngine* m_engine{nullptr};
        bool m_initialized{false};

        VulkanSwapBuffer m_buffer;
        VkBufferUsageFlags m_usage{0};

        size_t m_elementSize{0};
        size_t m_elementCount{0};
        size_t m_elementCapacity{0};

        void reallocateBuffer(VkCommandBuffer commandBuffer);

        bool reallocationRequired(size_t newElemCount) const {
            return m_elementCount + newElemCount >= m_elementCapacity;
        }
    };

    template<typename T>
    VulkanExpandableBuffer makeExpandableBuffer(
            VulkanEngine& engine,
            VkBufferUsageFlags usage,
            size_t initialCapacity = VulkanExpandableBuffer::DEFAULT_INITIAL_CAPACITY) {
        VulkanExpandableBuffer buffer;
        buffer.init(
                engine,
                usage,
                sizeof(T),
                initialCapacity);
        return buffer;
    }
}// namespace moe