#include "Render/Vulkan/Pipeline/SkyBoxPipeline.hpp"
#include "Render/Vulkan/VulkanCamera.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"
#include "Render/Vulkan/VulkanInitializers.hpp"
#include "Render/Vulkan/VulkanMaterialCache.hpp"
#include "Render/Vulkan/VulkanMeshCache.hpp"
#include "Render/Vulkan/VulkanPipeline.hpp"
#include "Render/Vulkan/VulkanRenderable.hpp"
#include "Render/Vulkan/VulkanUtils.hpp"


namespace moe {
    namespace Pipeline {
        void SkyBoxPipeline::init(VulkanEngine& engine) {
            MOE_ASSERT(!m_initialized, "SkyBoxPipeline already initialized");

            m_engine = &engine;
            m_initialized = true;

            auto vert = VkUtils::createShaderModuleFromFile(engine.m_device, "shaders/skybox.vert.spv");
            auto frag = VkUtils::createShaderModuleFromFile(engine.m_device, "shaders/skybox.frag.spv");

            VkPushConstantRange pushRange{
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(PushConstants),
            };

            auto pushRanges = Array<VkPushConstantRange, 1>{pushRange};
            auto descriptorLayouts = Array<VkDescriptorSetLayout, 1>{
                    engine.getBindlessSet().getDescriptorSetLayout(),
            };

            auto pipelineLayoutInfo = VkInit::pipelineLayoutCreateInfo(descriptorLayouts, pushRanges);
            MOE_VK_CHECK(vkCreatePipelineLayout(engine.m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

            auto builder = VulkanPipelineBuilder(m_pipelineLayout)
                                   .addShader(vert, frag)
                                   .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                   .setPolygonMode(VK_POLYGON_MODE_FILL)
                                   .setCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                   .disableBlending()
                                   .enableDepthTesting(true, VK_COMPARE_OP_LESS_OR_EQUAL)
                                   .setColorAttachmentFormat(engine.m_drawImage.imageFormat)
                                   .setDepthFormat(engine.m_depthImage.imageFormat);

            if (engine.isMultisamplingEnabled()) {
                builder.enableMultisampling(engine.m_msaaSamples);
            } else {
                builder.disableMultisampling();
            }

            m_pipeline = builder.build(engine.m_device);

            vkDestroyShaderModule(engine.m_device, vert, nullptr);
            vkDestroyShaderModule(engine.m_device, frag, nullptr);
        }

        void SkyBoxPipeline::draw(VkCommandBuffer commandBuffer, VulkanCamera& camera) {
            MOE_ASSERT(m_initialized, "SkyBoxPipeline not initialized");
            MOE_ASSERT(m_skyBoxImageId != NULL_IMAGE_ID, "SkyBox image not set");

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
            auto bindlessDescriptorSet = m_engine->getBindlessSet().getDescriptorSet();
            vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_pipelineLayout,
                    0, 1, &bindlessDescriptorSet,
                    0, nullptr);

            glm::mat4 view = glm::mat4(glm::mat3(camera.viewMatrix()));// remove translation

            auto extent = m_engine->m_drawExtent;
            float aspectRatio = (float) extent.width / (float) extent.height;

            glm::mat4 proj = camera.projectionMatrix(aspectRatio);
            //proj[1][1] *= -1;// vulkan clip space has inverted y and half z

            glm::mat4 viewProj = proj * view;
            glm::mat4 invViewProj = glm::inverse(viewProj);

            PushConstants pushConstants{
                    .inverseViewProj = invViewProj,
                    .cameraPos = glm::vec4(camera.getPosition(), 1.0f),
                    .skyBoxImageId = m_skyBoxImageId,
            };

            vkCmdPushConstants(
                    commandBuffer,
                    m_pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(PushConstants),
                    &pushConstants);

            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        }

        void SkyBoxPipeline::destroy() {
            MOE_ASSERT(m_initialized, "SkyBoxPipeline not initialized");

            vkDestroyPipeline(m_engine->m_device, m_pipeline, nullptr);
            vkDestroyPipelineLayout(m_engine->m_device, m_pipelineLayout, nullptr);

            m_initialized = false;
            m_engine = nullptr;
        }
    }// namespace Pipeline
}// namespace moe