#include "Render/Vulkan/VulkanDescriptors.hpp"

namespace moe {
    void VulkanDescriptorLayoutBuilder::addBinding(uint32_t binding, VkDescriptorType type) {
        VkDescriptorSetLayoutBinding bind{};
        bind.binding = binding;
        bind.descriptorCount = 1;
        bind.descriptorType = type;

        bindings.push_back(bind);
    }

    void VulkanDescriptorLayoutBuilder::clear() {
        bindings.clear();
    }

    VkDescriptorSetLayout VulkanDescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStage, void* next, VkDescriptorSetLayoutCreateFlags createFlags) {
        for (auto& binding: bindings) {
            binding.stageFlags |= shaderStage;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.pNext = next;

        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        layoutInfo.flags = createFlags;

        VkDescriptorSetLayout layout;
        MOE_VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout));

        return layout;
    }

    void VulkanDescriptorAllocator::initPool(VkDevice device, uint32_t maxSets, Span<VulkanDescriptorPoolSizeRatio> ratios) {
        Vector<VkDescriptorPoolSize> poolSizes;
        for (auto& ratio: ratios) {
            poolSizes.push_back({.type = ratio.type, .descriptorCount = static_cast<uint32_t>(ratio.ratio * maxSets)});
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.pNext = nullptr;

        poolInfo.flags = 0;
        poolInfo.maxSets = maxSets;

        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();

        vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
    }

    void VulkanDescriptorAllocator::clearDescriptors(VkDevice device) {
        vkResetDescriptorPool(device, descriptorPool, 0);
    }

    void VulkanDescriptorAllocator::destroyPool(VkDevice device) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }

    VkDescriptorSet VulkanDescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;

        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        VkDescriptorSet descriptor;
        MOE_VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptor));

        return descriptor;
    }

}// namespace moe