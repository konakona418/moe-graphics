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
        struct CSMPipeline {
        public:
            static constexpr uint32_t SHADOW_CASCADE_COUNT = 4;

            CSMPipeline() = default;
            ~CSMPipeline() = default;

            void init(VulkanEngine& engine, Array<float, SHADOW_CASCADE_COUNT> cascadeSplitRatios = {0.1f, 0.3f, 0.65f, 1.0f});

            void draw(
                    VkCommandBuffer cmdBuffer,
                    VulkanMeshCache& meshCache,
                    Span<VulkanRenderPacket> drawCommands,
                    const VulkanCamera& camera,
                    glm::vec3 lightDir);

            void destroy();

            ImageId getShadowMapImageId() const { return m_shadowMapImageId; }

            glm::vec3 getShadowMapCameraScale() const { return m_shadowMapCameraScale; }

            // x and y mainly affect shadow quality, z mainly affects shadow range
            // however if the scene is too large, a larger x and y may still be needed
            void setShadowMapCameraScale(glm::vec3 scale) { m_shadowMapCameraScale = scale; }

            glm::mat4 m_cascadeLightTransforms[SHADOW_CASCADE_COUNT];
            float m_cascadeFarPlaneZs[SHADOW_CASCADE_COUNT];

        private:
            struct PushConstants {
                glm::mat4 mvp;
                VkDeviceAddress vertexBufferAddr;
            };

            bool m_initialized{false};
            uint32_t m_csmShadowMapSize{4096};
            VulkanEngine* m_engine{nullptr};

            VkPipelineLayout m_pipelineLayout;
            VkPipeline m_pipeline;

            ImageId m_shadowMapImageId;
            Array<VkImageView, SHADOW_CASCADE_COUNT> m_shadowMapImageViews;

            Array<float, SHADOW_CASCADE_COUNT> m_cascadeSplitRatios;

            glm::vec3 m_shadowMapCameraScale{2.0f, 2.0f, 2.0f};
        };
    }// namespace Pipeline
}// namespace moe