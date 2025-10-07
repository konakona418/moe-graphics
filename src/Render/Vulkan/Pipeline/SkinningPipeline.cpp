#include "Render/Vulkan/Pipeline/SkinningPipeline.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"
#include "Render/Vulkan/VulkanInitializers.hpp"
#include "Render/Vulkan/VulkanMeshCache.hpp"
#include "Render/Vulkan/VulkanPipeline.hpp"
#include "Render/Vulkan/VulkanRenderable.hpp"
#include "Render/Vulkan/VulkanUtils.hpp"

namespace moe {
    namespace Pipeline {
        void SkinningPipeline::init(VulkanEngine& engine) {
            MOE_ASSERT(!m_initialized, "SkyBoxPipeline already initialized");

            m_engine = &engine;
            auto shader = VkUtils::createShaderModuleFromFile(m_engine->m_device, "shaders/skinning.comp.spv");

            auto pushRange = VkPushConstantRange{
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .offset = 0,
                    .size = sizeof(PushConstants),
            };

            auto descriptorSetLayout = engine.m_bindlessSet.getDescriptorSetLayout();

            auto pushRanges = Array<VkPushConstantRange, 1>{pushRange};
            auto descriptorLayouts = Array<VkDescriptorSetLayout, 1>{descriptorSetLayout};

            auto pipelineLayoutInfo = VkInit::pipelineLayoutCreateInfo(descriptorLayouts, pushRanges);
            MOE_VK_CHECK(vkCreatePipelineLayout(engine.m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

            m_pipeline = VulkanComputePipelineBuilder{m_pipelineLayout}
                                 .setShader(shader)
                                 .build(m_engine->m_device);

            vkDestroyShaderModule(engine.m_device, shader, nullptr);

            for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
                m_swapData[i].jointMatrixBuffer =
                        engine.allocateBuffer(
                                sizeof(glm::mat4) * MAX_JOINT_MATRIX_COUNT,
                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                VMA_MEMORY_USAGE_AUTO);
                m_swapData[i].jointMatrixBufferSize = 0;
            }

            m_initialized = true;
        }

        void SkinningPipeline::destroy() {
            MOE_ASSERT(m_initialized, "SkyBoxPipeline not initialized");

            for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
                m_engine->destroyBuffer(m_swapData[i].jointMatrixBuffer);
            }

            vkDestroyPipeline(m_engine->m_device, m_pipeline, nullptr);
            vkDestroyPipelineLayout(m_engine->m_device, m_pipelineLayout, nullptr);
        }

        void SkinningPipeline::beginFrame(size_t frameIndex) {
            MOE_ASSERT(m_initialized, "SkinningPipeline not initialized");
            MOE_ASSERT(frameIndex < FRAMES_IN_FLIGHT, "Invalid frame index");

            m_swapData[frameIndex].jointMatrixBufferSize = 0;
        }

        size_t SkinningPipeline::appendJointMatrices(Span<glm::mat4> jointMatrices, size_t frameIndex) {
            MOE_ASSERT(m_initialized, "SkinningPipeline is not initialized");
            MOE_ASSERT(frameIndex < FRAMES_IN_FLIGHT, "Invalid frame index");

            auto& swapData = m_swapData[frameIndex];

            size_t before = swapData.jointMatrixBufferSize;

            size_t jointMatrixCount = jointMatrices.size();
            size_t jointMatrixSize = jointMatrixCount * sizeof(glm::mat4);

            if (before + jointMatrixCount > MAX_JOINT_MATRIX_COUNT) {
                Logger::error("Exceeded maximum joint matrix count {}, clamping to max", MAX_JOINT_MATRIX_COUNT);
                return 0;
            }

            std::memcpy(
                    reinterpret_cast<glm::mat4*>(swapData.jointMatrixBuffer.vmaAllocationInfo.pMappedData) + before,
                    jointMatrices.data(),
                    jointMatrixSize);
            swapData.jointMatrixBufferSize += jointMatrixCount;

            return before;
        }

        void SkinningPipeline::compute(
                VkCommandBuffer cmdBuffer,
                VulkanMeshCache& meshCache,
                Span<VulkanRenderPacket> drawCommands,
                size_t frameIndex) {
            MOE_ASSERT(m_initialized, "SkinningPipeline is not initialized");

            auto& swapData = m_swapData[frameIndex];
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

            for (auto& packet: drawCommands) {
                if (!packet.skinned) {
                    continue;
                }

                if (packet.jointMatrixStartIndex == INVALID_JOINT_MATRIX_START_INDEX) {
                    Logger::warn("Skinned mesh draw command has invalid joint matrix start index, skipping. Note that the matrices must be uploaded before calling this function");
                    continue;
                }

                auto mesh = meshCache.getMesh(packet.meshId).value();

                auto pushConstants = PushConstants{
                        .vertexBufferAddr = mesh.gpuBuffer.vertexBufferAddr,
                        .skinningDataBufferAddr = mesh.gpuBuffer.skinningDataBufferAddr,
                        .jointMatrixBufferAddr = swapData.jointMatrixBuffer.address,
                        .outputVertexBufferAddr = mesh.gpuBuffer.skinnedVertexBufferAddr,
                        .jointMatrixStartIndex = (uint32_t) packet.jointMatrixStartIndex,
                        .vertexCount = mesh.gpuBuffer.vertexCount,
                };

                vkCmdPushConstants(cmdBuffer, m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &pushConstants);

                constexpr uint32_t workGroupSize = 128;
                const auto groupCount = (uint32_t) std::ceil(mesh.gpuBuffer.vertexCount / (float) workGroupSize);
                vkCmdDispatch(cmdBuffer, groupCount, 1, 1);
            }
        }
    }// namespace Pipeline
}// namespace moe