#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


namespace moe {
    class VulkanEngine;
    class VulkanCamera;
};// namespace moe

namespace moe {
    namespace Pipeline {
        struct SkyBoxPipeline {
            SkyBoxPipeline() = default;
            ~SkyBoxPipeline() = default;

            void init(VulkanEngine& engine);

            void draw(VkCommandBuffer commandBuffer, VulkanCamera& camera);

            void destroy();

            void setSkyBoxImage(ImageId imageId) {
                m_skyBoxImageId = imageId;
            }

        private:
            struct PushConstants {
                glm::mat4 inverseViewProj;
                glm::vec4 cameraPos;
                uint32_t skyBoxImageId;
            };

            VulkanEngine* m_engine{nullptr};
            bool m_initialized{false};

            VkPipelineLayout m_pipelineLayout;
            VkPipeline m_pipeline;

            ImageId m_skyBoxImageId{NULL_IMAGE_ID};
        };
    }// namespace Pipeline
}// namespace moe