#include "Render/Vulkan/VulkanBindlessSet.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"

namespace moe {
    void VulkanBindlessSet::init(VulkanEngine& engine, int maxAnisotropy) {
        MOE_ASSERT(!m_initialized, "VulkanBindlessSet already initialized");

        m_engine = &engine;
        m_initialized = true;

        {
            auto poolSizes = Array<VkDescriptorPoolSize, 2>{
                    VkDescriptorPoolSize{
                            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                            .descriptorCount = MAX_BINDLESS_IMAGES,
                    },
                    VkDescriptorPoolSize{
                            .type = VK_DESCRIPTOR_TYPE_SAMPLER,
                            .descriptorCount = MAX_BINDLESS_SAMPLERS,
                    },
            };

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
            poolInfo.maxSets = 1;
            poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            poolInfo.pPoolSizes = poolSizes.data();

            MOE_VK_CHECK(vkCreateDescriptorPool(m_engine->m_device, &poolInfo, nullptr, &m_descriptorPool));
        }

        {
            auto bindings = Array<VkDescriptorSetLayoutBinding, 2>{
                    VkDescriptorSetLayoutBinding{
                            .binding = IMAGE_BINDING_INDEX,
                            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                            .descriptorCount = MAX_BINDLESS_IMAGES,
                            .stageFlags = VK_SHADER_STAGE_ALL,
                    },
                    VkDescriptorSetLayoutBinding{
                            .binding = SAMPLER_BINDING_INDEX,
                            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                            .descriptorCount = MAX_BINDLESS_SAMPLERS,
                            .stageFlags = VK_SHADER_STAGE_ALL,
                    },
            };

            const auto bindingFlags = Array<VkDescriptorBindingFlags, 2>{
                    VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
                    VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
            };

            const auto bindingFlagInfo =
                    VkDescriptorSetLayoutBindingFlagsCreateInfo{
                            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
                            .bindingCount = bindingFlags.size(),
                            .pBindingFlags = bindingFlags.data(),
                    };

            VkDescriptorSetLayoutCreateInfo layoutInfo =
                    VkDescriptorSetLayoutCreateInfo{
                            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                            .pNext = &bindingFlagInfo,
                            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
                            .bindingCount = bindings.size(),
                            .pBindings = bindings.data(),
                    };

            MOE_VK_CHECK(vkCreateDescriptorSetLayout(m_engine->m_device, &layoutInfo, nullptr, &m_descriptorSetLayout));
        }

        {
            const auto allocInfo =
                    VkDescriptorSetAllocateInfo{
                            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                            .descriptorPool = m_descriptorPool,
                            .descriptorSetCount = 1,
                            .pSetLayouts = &m_descriptorSetLayout,
                    };

            /*
            uint32_t maxBinding = MAX_BINDLESS_IMAGES - 1;
            const auto variableDescCountInfo =
                    VkDescriptorSetVariableDescriptorCountAllocateInfo{
                            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
                            .descriptorSetCount = 1,
                            .pDescriptorCounts = &maxBinding,
                    };
            */

            MOE_VK_CHECK(vkAllocateDescriptorSets(m_engine->m_device, &allocInfo, &m_descriptorSet));
        }

        initDefaultSamplers(maxAnisotropy);
    }

    void VulkanBindlessSet::initDefaultSamplers(int maxAnisotropy) {
        {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

            samplerInfo.minFilter = VK_FILTER_NEAREST;
            samplerInfo.magFilter = VK_FILTER_NEAREST;

            vkCreateSampler(m_engine->m_device, &samplerInfo, nullptr, &m_defaultSamplers.nearestSampler);
            addSampler(0, m_defaultSamplers.nearestSampler);
        }

        {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            // todo: anisotropy

            vkCreateSampler(m_engine->m_device, &samplerInfo, nullptr, &m_defaultSamplers.linearSampler);
            addSampler(1, m_defaultSamplers.linearSampler);
        }
    }

    void VulkanBindlessSet::addImage(uint32_t id, VkImageView imageView) {
        MOE_ASSERT(m_initialized, "VulkanBindlessSet not initialized");
        MOE_ASSERT(id < MAX_BINDLESS_IMAGES, "Invalid image ID");

        const auto imageInfo = VkDescriptorImageInfo{
                .sampler = VK_NULL_HANDLE,
                .imageView = imageView,
                .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
        };

        const auto writeSet = VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSet,
                .dstBinding = IMAGE_BINDING_INDEX,
                .dstArrayElement = id,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = &imageInfo,
        };

        vkUpdateDescriptorSets(m_engine->m_device, 1, &writeSet, 0, nullptr);
    }

    void VulkanBindlessSet::addSampler(uint32_t id, VkSampler sampler) {
        MOE_ASSERT(m_initialized, "VulkanBindlessSet not initialized");
        MOE_ASSERT(id < MAX_BINDLESS_SAMPLERS, "Invalid sampler ID");

        const auto imageInfo = VkDescriptorImageInfo{
                .sampler = sampler,
                .imageView = VK_NULL_HANDLE,
                .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
        };

        const auto writeSet = VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSet,
                .dstBinding = SAMPLER_BINDING_INDEX,
                .dstArrayElement = id,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                .pImageInfo = &imageInfo,
        };

        vkUpdateDescriptorSets(m_engine->m_device, 1, &writeSet, 0, nullptr);
    }

    void VulkanBindlessSet::destroy() {
        MOE_ASSERT(m_initialized, "VulkanBindlessSet not initialized");

        vkDestroySampler(m_engine->m_device, m_defaultSamplers.nearestSampler, nullptr);
        vkDestroySampler(m_engine->m_device, m_defaultSamplers.linearSampler, nullptr);

        vkDestroyDescriptorSetLayout(m_engine->m_device, m_descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(m_engine->m_device, m_descriptorPool, nullptr);

        m_engine = nullptr;
        m_initialized = false;
    }
}// namespace moe