#pragma once

#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    namespace Pipeline {
        struct VulkanMeshPipeline {
            enum class MaterialPass : uint8_t {
                Opaque = 0,
                Transparent,
                Count
            };

            struct MaterialPipeline {
                VkPipeline pipeline;
                VkPipelineLayout layout;
            };

            struct MaterialInstance {
                MaterialPipeline* pipeline;
                VkDescriptorSet descriptorSet;
                MaterialPass pass;
            };
        };
    }// namespace Pipeline
}// namespace moe