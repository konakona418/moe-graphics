#pragma once

#include "Render/Vulkan/VulkanSwapBuffer.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

#include <im3d.h>

namespace moe {
    struct VulkanEngine;
    struct VulkanCamera;
}// namespace moe

namespace moe {
    struct VulkanIm3dDriver {
    public:
        struct MouseState {
            glm::vec2 position;
            bool leftButtonDown{false};
        };

        VulkanIm3dDriver() = default;
        ~VulkanIm3dDriver() = default;

        void init(
                VulkanEngine& engine,
                VkFormat drawImageFormat,
                VkFormat depthImageFormat);

        void beginFrame(
                glm::vec2 viewportSize,
                const VulkanCamera* camera,
                MouseState mouseState);

        void render(
                VkCommandBuffer cmdBuffer,
                const VulkanCamera* camera,
                VkImageView targetImageView,
                VkImageView depthImageView,
                VkExtent2D extent);

        void endFrame();

        void destroy();

    private:
        static constexpr size_t MAX_VERTEX_COUNT = 65536;

        struct PushConstants {
            VkDeviceAddress vertexBufferAddr;
            glm::mat4 viewProjection;
            glm::vec2 viewportSize;
        };

        VulkanEngine* m_engine{nullptr};
        bool m_initialized{false};

        VulkanSwapBuffer m_vertexBuffer;

        std::chrono::high_resolution_clock::time_point m_lastUploadTime;
        bool m_islastUploadTimeValid{false};

        struct {
            VkPipeline pointPipeline;
            VkPipelineLayout pointPipelineLayout;

            VkPipeline linePipeline;
            VkPipelineLayout linePipelineLayout;

            VkPipeline trianglePipeline;
            VkPipelineLayout trianglePipelineLayout;
        } m_pipelines;

        void uploadVertices(VkCommandBuffer cmdBuffer);
    };
}// namespace moe
