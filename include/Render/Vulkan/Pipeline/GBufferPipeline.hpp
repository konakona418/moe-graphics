#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanRenderable.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


namespace moe {
    class VulkanEngine;
    class VulkanMeshCache;
    class VulkanMaterialCache;
}// namespace moe


namespace moe {
    namespace Pipeline {
        struct GBufferPipeline {
            GBufferPipeline() = default;
            ~GBufferPipeline() = default;

            void init(VulkanEngine& engine);

            void destroy();

            void draw(
                    VkCommandBuffer cmdBuffer,
                    VulkanMeshCache& meshCache,
                    VulkanMaterialCache& materialCache,
                    Span<VulkanRenderPacket> drawCommands,
                    VulkanAllocatedBuffer& sceneDataBuffer);

            //VulkanAllocatedImage gPosition;
            VulkanAllocatedImage gDepth;
            VulkanAllocatedImage gNormal;
            VulkanAllocatedImage gAlbedo;
            VulkanAllocatedImage gORMA;// Occulusion, Roughness, Metallic, AO
            VulkanAllocatedImage gEmissive;

            ImageId gDepthId{NULL_IMAGE_ID};
            ImageId gNormalId{NULL_IMAGE_ID};
            ImageId gAlbedoId{NULL_IMAGE_ID};
            ImageId gORMAId{NULL_IMAGE_ID};
            ImageId gEmissiveId{NULL_IMAGE_ID};

        private:
            struct PushConstants {
                glm::mat4 transform;
                VkDeviceAddress vertexBufferAddr;
                VkDeviceAddress sceneDataAddress;
                MaterialId materialId;
            };

            VulkanEngine* m_engine{nullptr};
            bool m_initialized{false};

            VkPipelineLayout m_pipelineLayout;
            VkPipeline m_pipeline;

            void allocateImages();

            void transitionImagesForRendering(VkCommandBuffer cmdBuffer);

            void transitionImagesForSampling(VkCommandBuffer cmdBuffer);
        };
    }// namespace Pipeline
}// namespace moe