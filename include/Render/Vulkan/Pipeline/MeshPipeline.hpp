#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanRenderable.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


// fwd decl
namespace moe {
    class VulkanEngine;
    class VulkanMeshCache;
    class VulkanMaterialCache;
}// namespace moe

namespace moe {
    namespace Pipeline {
        struct VulkanMeshPipeline {
        public:
            VulkanMeshPipeline() = default;
            ~VulkanMeshPipeline() = default;

            void init(VulkanEngine& engine);

            void draw(
                    VkCommandBuffer cmdBuffer,
                    VulkanMeshCache& meshCache,
                    VulkanMaterialCache& materialCache,
                    Span<VulkanRenderPacket> drawCommands,
                    VulkanAllocatedBuffer& sceneDataBuffer);

            void destroy();

        private:
            struct PushConstants {
                glm::mat4 transform;
                VkDeviceAddress vertexBufferAddr;
                VkDeviceAddress sceneDataAddress;
                MaterialId materialId;
            };

            bool m_initialized{false};
            VulkanEngine* m_engine{nullptr};

            VkPipelineLayout m_pipelineLayout;
            VkPipeline m_pipeline;
        };
    }// namespace Pipeline
}// namespace moe