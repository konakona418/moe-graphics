#pragma once


#include "Render/Vulkan/VulkanEngine.hpp"
#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanRenderable.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    class VulkanEngine;
    class VulkanMeshCache;
}// namespace moe

namespace moe {
    namespace Pipeline {
        struct SkinningPipeline {
        public:
            SkinningPipeline() = default;
            ~SkinningPipeline() = default;

            void init(VulkanEngine& engine);

            void compute(
                    VkCommandBuffer cmdBuffer,
                    VulkanMeshCache& meshCache,
                    Span<VulkanRenderPacket> drawCommands,
                    size_t frameIndex);

            void destroy();

        private:
            static constexpr uint32_t MAX_JOINT_MATRIX_COUNT = 1024;

            struct PushConstants {
                VkDeviceAddress vertexBufferAddr;
                VkDeviceAddress skinningDataBufferAddr;
                VkDeviceAddress jointMatrixBufferAddr;
                VkDeviceAddress outputVertexBufferAddr;
            };

            struct SwapData {
                VulkanAllocatedBuffer jointMatrixBuffer;
            };

            Array<SwapData, FRAMES_IN_FLIGHT> m_swapData;

            VulkanEngine* m_engine{nullptr};
            bool m_initialized{false};

            VkPipelineLayout m_pipelineLayout;
            VkPipeline m_pipeline;
        };
    }// namespace Pipeline
}// namespace moe