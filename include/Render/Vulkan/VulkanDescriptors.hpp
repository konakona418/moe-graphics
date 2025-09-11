#pragma once

#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    struct VulkanDescriptorLayoutBuilder {
        Vector<VkDescriptorSetLayoutBinding> bindings;

        void addBinding(uint32_t binding, VkDescriptorType type);
        void clear();
        VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStage, void* next = nullptr, VkDescriptorSetLayoutCreateFlags createFlags = 0);
    };

    struct VulkanDescriptorAllocator {
        struct VulkanDescriptorPoolSizeRatio {
            VkDescriptorType type;
            float ratio;
        };

        VkDescriptorPool descriptorPool;

        void initPool(VkDevice device, uint32_t maxSets, Span<VulkanDescriptorPoolSizeRatio> ratios);

        void clearDescriptors(VkDevice device);

        void destroyPool(VkDevice device);

        VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
    };

}// namespace moe