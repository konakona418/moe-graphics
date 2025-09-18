#pragma once

#include "Render/Vulkan/VulkanTypes.hpp"

// fwd decl
namespace moe {
    class VulkanEngine;
}

namespace moe {
    struct VulkanBindlessSet {
    public:
        static constexpr uint32_t MAX_BINDLESS_IMAGES = 8192;
        static constexpr uint32_t MAX_BINDLESS_SAMPLERS = 32;

        // the binding index must match that in the shaders
        static constexpr uint32_t IMAGE_BINDING_INDEX = 0;
        static constexpr uint32_t SAMPLER_BINDING_INDEX = 1;

        VulkanBindlessSet() = default;
        ~VulkanBindlessSet() = default;

        // todo: maxAnistropy
        void init(VulkanEngine& engine, int maxAnisotropy = 1);

        void addImage(uint32_t id, VkImageView imageView);

        void addSampler(uint32_t id, VkSampler sampler);

        void destroy();

        VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout; }

        VkDescriptorSet getDescriptorSet() const { return m_descriptorSet; }

        bool isInitialized() const { return m_initialized; }

    private:
        VulkanEngine* m_engine;
        bool m_initialized{false};

        VkDescriptorPool m_descriptorPool;
        VkDescriptorSetLayout m_descriptorSetLayout;
        VkDescriptorSet m_descriptorSet;

        struct {
            VkSampler nearestSampler;
            VkSampler linearSampler;
        } m_defaultSamplers;

        void initDefaultSamplers(int maxAnisotropy);
    };
}// namespace moe