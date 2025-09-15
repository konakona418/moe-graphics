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

    void VulkanDescriptorAllocator::init(VkDevice device, uint32_t maxSets, Span<PoolSizeRatio> ratios) {
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

    void VulkanDescriptorAllocator::clearPool(VkDevice device) {
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

    void VulkanDescriptorAllocatorDynamic::init(VkDevice device, uint32_t maxSets, Span<PoolSizeRatio> ratios) {
        m_poolSizeRatios.clear();

        for (auto& ratio: ratios) {
            m_poolSizeRatios.push_back(ratio);
        }

        auto pool = createPool(device, maxSets, ratios);
        m_setsPerPool = maxSets * POOL_SIZE_GROW_FACTOR;

        m_readyPools.push_back(pool);
    }

    void VulkanDescriptorAllocatorDynamic::clearPools(VkDevice device) {
        for (auto& pool: m_usedPools) {
            vkResetDescriptorPool(device, pool, 0);
        }

        for (auto& pool: m_usedPools) {
            vkResetDescriptorPool(device, pool, 0);
            m_readyPools.push_back(pool);
        }

        m_usedPools.clear();
    }

    void VulkanDescriptorAllocatorDynamic::destroyPools(VkDevice device) {
        for (auto& pool: m_usedPools) {
            vkDestroyDescriptorPool(device, pool, nullptr);
        }
        m_usedPools.clear();

        for (auto& pool: m_readyPools) {
            vkDestroyDescriptorPool(device, pool, nullptr);
        }
        m_readyPools.clear();
    }

    VkDescriptorSet VulkanDescriptorAllocatorDynamic::allocate(VkDevice device, VkDescriptorSetLayout layout, void* next) {
        VkDescriptorPool pool = getPool(device);

        MOE_ASSERT(pool != VK_NULL_HANDLE, "Failed to get a valid descriptor pool");

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext = next;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        VkDescriptorSet descriptorSet;
        auto result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

        if (result != VK_SUCCESS) {
            m_usedPools.push_back(pool);

            pool = getPool(device);
            allocInfo.descriptorPool = pool;

            MOE_VK_CHECK_MSG(
                    vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet),
                    "Descriptor allocation failed too many times");
        }

        m_readyPools.push_back(pool);

        return descriptorSet;
    }

    VkDescriptorPool VulkanDescriptorAllocatorDynamic::getPool(VkDevice device) {
        VkDescriptorPool pool;
        if (m_readyPools.size() > 0) {
            pool = m_readyPools.back();
            m_readyPools.pop_back();
        } else {
            pool = createPool(device, m_setsPerPool, m_poolSizeRatios);
            m_setsPerPool = m_setsPerPool * POOL_SIZE_GROW_FACTOR;

            if (m_setsPerPool > MAX_SETS_PER_POOL) {
                m_setsPerPool = MAX_SETS_PER_POOL;
            }
        }
        return pool;
    }

    VkDescriptorPool VulkanDescriptorAllocatorDynamic::createPool(VkDevice device, uint32_t maxSets, Span<PoolSizeRatio> ratios) {
        Vector<VkDescriptorPoolSize> poolSizes;
        for (auto& ratio: ratios) {
            poolSizes.push_back(
                    {
                            .type = ratio.type,
                            .descriptorCount = static_cast<uint32_t>(ratio.ratio * maxSets),
                    });
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.pNext = nullptr;

        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = maxSets;

        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();

        VkDescriptorPool descriptorPool;
        MOE_VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));

        return descriptorPool;
    }

    VulkanDescriptorWriter& VulkanDescriptorWriter::writeImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkDescriptorType type) {
        auto& imageInfo =
                m_imageInfos.emplace_back(VkDescriptorImageInfo{
                        .sampler = sampler,
                        .imageView = imageView,
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                });

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

        write.dstSet = VK_NULL_HANDLE;
        write.dstBinding = binding;

        write.descriptorCount = 1;
        write.descriptorType = type;
        write.pImageInfo = &imageInfo;

        m_writes.push_back(write);

        return *this;
    }

    VulkanDescriptorWriter& VulkanDescriptorWriter::writeBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset, VkDescriptorType type) {
        auto& bufferInfo =
                m_bufferInfos.emplace_back(VkDescriptorBufferInfo{
                        .buffer = buffer,
                        .offset = offset,
                        .range = size,
                });

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

        write.dstSet = VK_NULL_HANDLE;
        write.dstBinding = binding;

        write.descriptorCount = 1;
        write.descriptorType = type;
        write.pBufferInfo = &bufferInfo;

        m_writes.push_back(write);

        return *this;
    }

    void VulkanDescriptorWriter::clear() {
        m_imageInfos.clear();
        m_bufferInfos.clear();
        m_writes.clear();
    }

    VulkanDescriptorWriter& VulkanDescriptorWriter::updateSet(VkDevice device, VkDescriptorSet set) {
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0, nullptr);
        return *this;
    }
}// namespace moe