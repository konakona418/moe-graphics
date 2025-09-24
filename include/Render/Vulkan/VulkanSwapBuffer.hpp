#pragma once

#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    class VulkanEngine;
}// namespace moe

namespace moe {
    struct VulkanSwapBuffer {
    public:
        VulkanSwapBuffer() = default;
        ~VulkanSwapBuffer() = default;

        void init(VulkanEngine& engine, VkBufferUsageFlags usage, size_t bufferSize, size_t swapCount);

        void destroy();

        void upload(VkCommandBuffer cmdBuffer, void* data, size_t swapIndex, size_t size, size_t offset = 0);

        VulkanAllocatedBuffer& getBuffer() { return m_buffer; }

    private:
        size_t m_bufferSize{0};
        size_t m_swapCount{0};
        bool m_initialized{false};
        VulkanEngine* m_engine{nullptr};

        VulkanAllocatedBuffer m_buffer;
        Vector<VulkanAllocatedBuffer> m_stagingBuffers;
    };
}// namespace moe