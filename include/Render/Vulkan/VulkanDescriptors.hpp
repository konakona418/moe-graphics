#pragma once

#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    struct VulkanDescriptorLayoutBuilder {
        Vector<VkDescriptorSetLayoutBinding> bindings;

        VulkanDescriptorLayoutBuilder& addBinding(uint32_t binding, VkDescriptorType type);
        void clear();
        VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStage, void* next = nullptr, VkDescriptorSetLayoutCreateFlags createFlags = 0);
    };

    struct VulkanDescriptorAllocator {
        struct PoolSizeRatio {
            VkDescriptorType type;
            float ratio;
        };

        VkDescriptorPool descriptorPool;

        void init(VkDevice device, uint32_t maxSets, Span<PoolSizeRatio> ratios);

        void clearPool(VkDevice device);

        void destroyPool(VkDevice device);

        VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
    };

    struct VulkanDescriptorAllocatorDynamic {
        static constexpr uint32_t MAX_SETS_PER_POOL = 4096;
        static constexpr float POOL_SIZE_GROW_FACTOR = 2.0f;

        struct PoolSizeRatio {
            VkDescriptorType type;
            float ratio;
        };

        void init(VkDevice device, uint32_t maxSets, Span<PoolSizeRatio> ratios);

        void clearPools(VkDevice device);

        void destroyPools(VkDevice device);

        VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void* next = VK_NULL_HANDLE);

        VkDescriptorPool getPool(VkDevice device);

        VkDescriptorPool createPool(VkDevice device, uint32_t maxSets, Span<PoolSizeRatio> ratios);

        Vector<PoolSizeRatio> m_poolSizeRatios;
        Vector<VkDescriptorPool> m_readyPools;
        Vector<VkDescriptorPool> m_usedPools;

        uint32_t m_setsPerPool;
    };

    struct VulkanDescriptorWriter {
        Deque<VkDescriptorImageInfo> m_imageInfos;
        Deque<VkDescriptorBufferInfo> m_bufferInfos;
        Vector<VkWriteDescriptorSet> m_writes;

        VulkanDescriptorWriter& writeImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkDescriptorType type);
        VulkanDescriptorWriter& writeBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset, VkDescriptorType type);

        void clear();
        VulkanDescriptorWriter& updateSet(VkDevice device, VkDescriptorSet set);
    };
}// namespace moe