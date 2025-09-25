#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


// fwd decl
namespace moe {
    class VulkanEngine;
    class VulkanMeshCache;
    class VulkanMaterialCache;
    class VulkanRenderPacket;
    class VulkanCamera;
}// namespace moe

namespace moe {
    namespace Pipeline {
        struct ShadowMapPipeline {
        public:
            static constexpr uint32_t SHADOW_CASCADE_COUNT = 4;

            ShadowMapPipeline() = default;
            ~ShadowMapPipeline() = default;

            void init(VulkanEngine& engine);

            void draw(
                    VkCommandBuffer cmdBuffer,
                    VulkanMeshCache& meshCache,
                    Span<VulkanRenderPacket> drawCommands,
                    const VulkanCamera& camera,
                    glm::vec3 lightDir);

            void destroy();

            ImageId getShadowMapImageId() const { return m_shadowMapImageId; }

            glm::mat4 getShadowMapLightTransform() const { return m_shadowMapLightTransform; }

        private:
            struct PushConstants {
                glm::mat4 mvp;
                VkDeviceAddress vertexBufferAddr;
            };

            bool m_initialized{false};
            uint32_t m_shadowMapSize{2048};
            VulkanEngine* m_engine{nullptr};

            VkPipelineLayout m_pipelineLayout;
            VkPipeline m_pipeline;

            ImageId m_shadowMapImageId;
            VkImageView m_shadowMapImageView;

            glm::mat4 m_shadowMapLightTransform;
        };
    }// namespace Pipeline
}// namespace moe