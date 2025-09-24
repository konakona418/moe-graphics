#include "Render/Vulkan/Pipeline/DeferredLightingPipeline.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"
#include "Render/Vulkan/VulkanInitializers.hpp"
#include "Render/Vulkan/VulkanPipeline.hpp"
#include "Render/Vulkan/VulkanUtils.hpp"

namespace moe {
    namespace Pipeline {
        void DeferredLightingPipeline::init(VulkanEngine& engine) {
            MOE_ASSERT(!m_initialized, "DeferredLightingPipeline already initialized");

            m_initialized = true;
            m_engine = &engine;

            auto vert =
                    VkUtils::createShaderModuleFromFile(engine.m_device, "shaders/deferred_lighting.vert.spv");
            auto frag =
                    VkUtils::createShaderModuleFromFile(engine.m_device, "shaders/deferred_lighting.frag.spv");

            auto pushRange = VkPushConstantRange{
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(PushConstants),
            };

            auto pushRanges =
                    Array<VkPushConstantRange, 1>{pushRange};

            VulkanDescriptorLayoutBuilder layoutBuilder{};

            auto descriptorLayouts = Array<VkDescriptorSetLayout, 1>{
                    engine.getBindlessSet().getDescriptorSetLayout(),
            };

            auto pipelineLayoutInfo =
                    VkInit::pipelineLayoutCreateInfo(descriptorLayouts, pushRanges);

            MOE_VK_CHECK(vkCreatePipelineLayout(engine.m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

            auto builder = VulkanPipelineBuilder(m_pipelineLayout)
                                   .addShader(vert, frag)
                                   .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                   .setPolygonMode(VK_POLYGON_MODE_FILL)
                                   .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                   .disableBlending()
                                   .enableDepthTesting(VK_TRUE, VK_COMPARE_OP_LESS)
                                   .setColorAttachmentFormat(engine.m_drawImage.imageFormat)
                                   .disableMultisampling();

            if (engine.isMultisamplingEnabled()) {
                Logger::error("Enabling multisampling in deferred rendering is of no use");
                MOE_ASSERT(false, "Enabling multisampling in deferred rendering is of no use");
            } else {
                builder.disableMultisampling();
            }

            m_pipeline = builder.build(engine.m_device);

            vkDestroyShaderModule(engine.m_device, vert, nullptr);
            vkDestroyShaderModule(engine.m_device, frag, nullptr);
        }

        void DeferredLightingPipeline::destroy() {
            MOE_ASSERT(m_initialized, "DeferredLightingPipeline not initialized");

            vkDestroyPipeline(m_engine->m_device, m_pipeline, nullptr);
            vkDestroyPipelineLayout(m_engine->m_device, m_pipelineLayout, nullptr);

            m_initialized = false;
            m_engine = nullptr;
        }

        void DeferredLightingPipeline::draw(
                VkCommandBuffer cmdBuffer,
                VulkanAllocatedBuffer& sceneDataBuffer,
                ImageId gDepthId,
                ImageId gAlbedoId,
                ImageId gNormalId,
                ImageId gORMAId,
                ImageId gEmissiveId) {
            MOE_ASSERT(m_initialized, "DeferredLightingPipeline not initialized");

            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

            auto bindlessDescriptorSet = m_engine->getBindlessSet().getDescriptorSet();
            vkCmdBindDescriptorSets(
                    cmdBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_pipelineLayout,
                    0, 1,
                    &bindlessDescriptorSet,
                    0, nullptr);

            const auto viewport = VkViewport{
                    .x = 0,
                    .y = 0,
                    .width = (float) m_engine->m_drawExtent.width,
                    .height = (float) m_engine->m_drawExtent.height,
                    .minDepth = 0.f,
                    .maxDepth = 1.f,
            };
            vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

            VkRect2D scissor = {.offset = {0, 0}, .extent = m_engine->m_drawExtent};
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

            const auto pushConstants = PushConstants{
                    .sceneDataAddress = sceneDataBuffer.address,
                    .gDepthId = gDepthId,
                    .gAlbedoId = gAlbedoId,
                    .gNormalId = gNormalId,
                    .gORMAId = gORMAId,
                    .gEmissiveId = gEmissiveId,
            };

            vkCmdPushConstants(
                    cmdBuffer,
                    m_pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0, sizeof(PushConstants),
                    &pushConstants);

            // draw a full-screen triangle
            vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
        }
    }// namespace Pipeline
}// namespace moe