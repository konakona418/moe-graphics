#include "Render/Vulkan/VulkanSwapBuffer.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"

namespace moe {
    void VulkanSwapBuffer::init(VulkanEngine& engine, VkBufferUsageFlags usage, size_t bufferSize, size_t swapCount) {
        MOE_ASSERT(!m_initialized, "VulkanSwapBuffer already initialized");
        MOE_ASSERT(swapCount > 0, "Swap count must be greater than 0");
        MOE_ASSERT(bufferSize > 0, "Buffer size must be greater than 0");

        m_bufferSize = bufferSize;
        m_swapCount = swapCount;
        m_engine = &engine;

        m_buffer = engine.allocateBuffer(
                bufferSize * swapCount,
                usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);

        m_stagingBuffers.resize(swapCount);
        for (size_t i = 0; i < swapCount; i++) {
            m_stagingBuffers[i] = engine.allocateBuffer(
                    bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        m_initialized = true;
    }

    void VulkanSwapBuffer::destroy() {
        MOE_ASSERT(m_initialized, "Swap buffer not initialized");

        for (auto& buffer: m_stagingBuffers) {
            m_engine->destroyBuffer(buffer);
        }
        m_stagingBuffers.clear();
        m_engine->destroyBuffer(m_buffer);

        m_engine = nullptr;
        m_initialized = false;
    }

    void VulkanSwapBuffer::upload(VkCommandBuffer cmdBuffer, void* data, size_t swapIndex, size_t size, size_t offset) {
        MOE_ASSERT(m_initialized, "VulkanSwapBuffer not initialized");
        MOE_ASSERT(swapIndex < m_swapCount, "Swap index out of range");
        MOE_ASSERT(size + offset <= m_bufferSize, "Upload size exceeds buffer size");

        {
            auto barrier = VkBufferMemoryBarrier2{
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                    .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                    .srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                    .buffer = m_buffer.buffer,
                    .offset = 0,
                    .size = VK_WHOLE_SIZE,
            };

            auto dependency = VkDependencyInfo{
                    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .bufferMemoryBarrierCount = 1,
                    .pBufferMemoryBarriers = &barrier,
            };

            vkCmdPipelineBarrier2(cmdBuffer, &dependency);
            // sync with last read
        }

        auto& stagingBuffer = m_stagingBuffers[swapIndex];
        auto* mapped = static_cast<uint8_t*>(stagingBuffer.vmaAllocationInfo.pMappedData);
        std::memcpy(mapped + offset, data, size);

        auto copyRegion = VkBufferCopy2{
                .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
                .srcOffset = (VkDeviceSize) offset,
                .dstOffset = (VkDeviceSize) offset,
                .size = size,
        };
        auto copyInfo = VkCopyBufferInfo2{
                .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                .srcBuffer = stagingBuffer.buffer,
                .dstBuffer = m_buffer.buffer,
                .regionCount = 1,
                .pRegions = &copyRegion,
        };

        vkCmdCopyBuffer2(cmdBuffer, &copyInfo);

        {
            auto barrier = VkBufferMemoryBarrier2{
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                    .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                    .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                    .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
                    .buffer = m_buffer.buffer,
                    .offset = 0,
                    .size = VK_WHOLE_SIZE,
            };

            auto dependency = VkDependencyInfo{
                    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .bufferMemoryBarrierCount = 1,
                    .pBufferMemoryBarriers = &barrier,
            };

            vkCmdPipelineBarrier2(cmdBuffer, &dependency);
            // sync
        }
    }
}// namespace moe