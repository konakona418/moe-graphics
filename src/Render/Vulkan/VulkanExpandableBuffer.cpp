#include "Render/Vulkan/VulkanExpandableBuffer.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"

namespace moe {
    void VulkanExpandableBuffer::init(
            VulkanEngine& engine,
            VkBufferUsageFlags usage,
            size_t elementSize,
            size_t initialCapacity) {
        MOE_ASSERT(!m_initialized, "VulkanExpandableBuffer already initialized");
        MOE_ASSERT(elementSize % 16 == 0, "Element must satisfy minimum alignment of 16 bytes");

        m_engine = &engine;
        m_elementSize = elementSize;
        m_elementCapacity = initialCapacity;
        m_elementCount = 0;

        m_buffer.init(
                engine,
                usage,
                m_elementSize * m_elementCapacity,
                1);
        m_usage = usage;

        m_initialized = true;
    }

    void VulkanExpandableBuffer::destroy() {
        MOE_ASSERT(m_initialized, "VulkanExpandableBuffer not initialized");

        m_buffer.destroy();

        m_initialized = false;
        m_engine = nullptr;
        m_elementSize = 0;
        m_elementCapacity = 0;
        m_elementCount = 0;
    }

    void VulkanExpandableBuffer::pushBack(VkCommandBuffer commandBuffer, void* data, size_t elemCount) {
        MOE_ASSERT(m_initialized, "VulkanExpandableBuffer not initialized");

        if (elemCount == 0) {
            return;
        }

        if (reallocationRequired(elemCount)) {
            reallocateBuffer(commandBuffer);
        }

        m_buffer.upload(
                commandBuffer,
                data,
                1,
                m_elementSize * elemCount,
                m_elementSize * m_elementCount);

        m_elementCount += elemCount;
    }

    void VulkanExpandableBuffer::reallocateBuffer(VkCommandBuffer commandBuffer) {
        MOE_ASSERT(m_initialized, "VulkanExpandableBuffer not initialized");

        size_t newCapacity = m_elementCapacity * DEFAULT_EXPANSION_FACTOR;
        VulkanSwapBuffer newBuffer;
        newBuffer.init(
                *m_engine,
                m_usage,
                m_elementSize * newCapacity,
                1);

        // copy data
        {
            VkBufferCopy2 copy{};
            copy.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
            copy.srcOffset = 0;
            copy.dstOffset = 0;
            copy.size = m_elementSize * m_elementCount;

            VkCopyBufferInfo2 copyInfo{};
            copyInfo.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
            copyInfo.srcBuffer = m_buffer.getBuffer().buffer;
            copyInfo.dstBuffer = newBuffer.getBuffer().buffer;
            copyInfo.regionCount = 1;
            copyInfo.pRegions = &copy;

            vkCmdCopyBuffer2(commandBuffer, &copyInfo);
        }

        m_buffer.destroy();
        m_buffer = std::move(newBuffer);
        m_elementCapacity = newCapacity;
    }
}// namespace moe