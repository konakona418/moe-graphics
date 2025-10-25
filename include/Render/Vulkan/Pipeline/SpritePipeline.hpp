#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanRenderable.hpp"
#include "Render/Vulkan/VulkanSprite.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


namespace moe {
    class VulkanEngine;
    class VulkanMeshCache;
    class VulkanMaterialCache;
}// namespace moe


namespace moe {
    namespace Pipeline {
        struct SpritePipeline {
        public:
            SpritePipeline() = default;
            ~SpritePipeline() = default;

            void init(VulkanEngine& engine);

            void draw(
                    VkCommandBuffer cmdBuffer,
                    VulkanMeshCache& meshCache,
                    Span<VulkanSprite> sprites,
                    const glm::mat4& viewProj,
                    VulkanAllocatedImage& renderTarget);

            void destroy();

        private:
            struct PushConstants {
                glm::mat4 transform;

                glm::vec4 color;
                glm::vec2 spriteSize;
                glm::vec2 texRegionOffset;
                glm::vec2 texRegionSize;
                glm::vec2 textureSize;
                uint32_t textureId;
                uint32_t isTextSprite;
            };

            VulkanEngine* m_engine{nullptr};
            bool m_initialized{false};

            VkPipelineLayout m_pipelineLayout;
            VkPipeline m_pipeline;
        };
    }// namespace Pipeline
}// namespace moe