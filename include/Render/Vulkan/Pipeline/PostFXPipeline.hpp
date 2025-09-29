#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    class VulkanEngine;
}


namespace moe {
    namespace Pipeline {
        struct FXAAPipeline {
        public:
            FXAAPipeline() = default;
            ~FXAAPipeline() = default;

            void init(VulkanEngine& engine);

            void draw(
                    VkCommandBuffer cmdBuffer,
                    ImageId inputImageId);

            void destroy();

        private:
            struct PushConstants {
                ImageId inputImageId;
                glm::vec2 screenSize;
                glm::vec2 inverseScreenSize;
            };

            VulkanEngine* m_engine{nullptr};
            bool m_initialized{false};

            VkPipelineLayout m_pipelineLayout;
            VkPipeline m_pipeline;
        };
    }// namespace Pipeline
}// namespace moe