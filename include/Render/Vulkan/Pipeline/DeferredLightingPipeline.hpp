#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    class VulkanEngine;
    class VulkanMeshCache;
    class VulkanMaterialCache;
}// namespace moe

namespace moe {
    namespace Pipeline {
        struct DeferredLightingPipeline {
        public:
            DeferredLightingPipeline() = default;
            ~DeferredLightingPipeline() = default;

            void init(VulkanEngine& engine);

            void draw(
                    VkCommandBuffer cmdBuffer,
                    VulkanAllocatedBuffer& sceneDataBuffer,
                    ImageId gDepthId,
                    ImageId gAlbedoId,
                    ImageId gNormalId,
                    ImageId gORMAId,
                    ImageId gEmissiveId);

            void destroy();

        private:
            struct PushConstants {
                VkDeviceAddress sceneDataAddress;
                ImageId gDepthId;
                ImageId gAlbedoId;
                ImageId gNormalId;
                ImageId gORMAId;
                ImageId gEmissiveId;
            };

            VulkanEngine* m_engine{nullptr};
            bool m_initialized{false};

            VkPipelineLayout m_pipelineLayout;
            VkPipeline m_pipeline;
        };
    }// namespace Pipeline
}// namespace moe
