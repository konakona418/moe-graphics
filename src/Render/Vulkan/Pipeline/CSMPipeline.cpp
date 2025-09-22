#include "Render/Vulkan/Pipeline/CSMPipeline.hpp"
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
        void CSMPipeline::init(VulkanEngine& engine, Array<float, SHADOW_CASCADE_COUNT> cascadeSplitRatios) {
            MOE_ASSERT(!m_initialized, "CSMPipeline is already initialized");

            m_engine = &engine;
            m_cascadeSplitRatios = cascadeSplitRatios;
            m_initialized = true;

            auto vert =
                    VkUtils::createShaderModuleFromFile(
                            m_engine->m_device, "shaders/mesh_depth.vert.spv");
            auto frag =
                    VkUtils::createShaderModuleFromFile(
                            m_engine->m_device, "shaders/mesh_depth.frag.spv");

            auto pushRange = VkPushConstantRange{
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(PushConstants),
            };

            auto ranges = Array<VkPushConstantRange, 1>{pushRange};
            auto descriptorLayouts = Array<VkDescriptorSetLayout, 1>{
                    engine.getBindlessSet().getDescriptorSetLayout(),
            };

            auto pipelineLayout =
                    VkInit::pipelineLayoutCreateInfo(descriptorLayouts, ranges);
            MOE_VK_CHECK(vkCreatePipelineLayout(engine.m_device, &pipelineLayout, nullptr, &m_pipelineLayout));

            m_pipeline =
                    VulkanPipelineBuilder(m_pipelineLayout)
                            .addShader(vert, frag)
                            .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                            .setPolygonMode(VK_POLYGON_MODE_FILL)
                            // ! use front face culling for shadow map
                            .setCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                            .disableMultisampling()
                            .disableBlending()
                            .enableDepthTesting(true, VK_COMPARE_OP_LESS)
                            .setDepthFormat(VK_FORMAT_D32_SFLOAT)
                            .build(engine.m_device);

            auto csmImageInfo =
                    VkInit::imageCreateInfo(
                            VK_FORMAT_D32_SFLOAT,
                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                            {m_csmShadowMapSize, m_csmShadowMapSize, 1});

            csmImageInfo.mipLevels = 1;
            csmImageInfo.arrayLayers = SHADOW_CASCADE_COUNT;

            auto image = engine.allocateImage(csmImageInfo);
            m_shadowMapImageId = engine.m_caches.imageCache.addImage(std::move(image));

            auto* csmImageRef = engine.m_caches.imageCache.getImage(m_shadowMapImageId).value();

            for (uint32_t i = 0; i < SHADOW_CASCADE_COUNT; ++i) {
                VkImageViewCreateInfo viewInfo = VkInit::imageViewCreateInfo(
                        VK_FORMAT_D32_SFLOAT,
                        csmImageRef->image,
                        VK_IMAGE_ASPECT_DEPTH_BIT);
                viewInfo.subresourceRange.baseArrayLayer = i;
                viewInfo.subresourceRange.layerCount = 1;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.baseMipLevel = 0;

                VkImageView imageView;
                MOE_VK_CHECK(vkCreateImageView(engine.m_device, &viewInfo, nullptr, &imageView));
                m_shadowMapImageViews[i] = imageView;
            }
        }

        void CSMPipeline::draw(
                VkCommandBuffer cmdBuffer,
                VulkanMeshCache& meshCache,
                VulkanMaterialCache& materialCache,
                Span<VulkanRenderPacket> drawCommands,
                VulkanAllocatedBuffer& sceneDataBuffer,
                const VulkanCamera& camera) {
            MOE_ASSERT(m_initialized, "CSMPipeline is not initialized");

            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
            auto bindlessDescriptorSet = m_engine->getBindlessSet().getDescriptorSet();
            vkCmdBindDescriptorSets(
                    cmdBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_pipelineLayout,
                    0, 1, &bindlessDescriptorSet,
                    0, nullptr);

            auto csmShadowMapResult = m_engine->m_caches.imageCache.getImage(m_shadowMapImageId);
            MOE_ASSERT(csmShadowMapResult.has_value(), "Invalid CSM shadow map image id");
            auto* csmShadowMap = csmShadowMapResult.value();

            VkUtils::transitionImage(
                    cmdBuffer,
                    csmShadowMap->image,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

            float nearZ = camera.getNearZ();
            float farZ = camera.getFarZ();

            for (int i = 0; i < SHADOW_CASCADE_COUNT; ++i) {
                float cascadeNearZ = i == 0 ? nearZ : nearZ * m_cascadeSplitRatios[i - 1];
                float cascadeFarZ = farZ * m_cascadeSplitRatios[i];

                VulkanCamera subFrustumCamera{
                        camera.getPosition(),
                        camera.getPitch(),
                        camera.getYaw(),
                        camera.getFovDeg(),
                        cascadeNearZ,
                        cascadeFarZ,
                };
            }
            // todo
        }

        void CSMPipeline::destroy() {
            MOE_ASSERT(m_initialized, "CSMPipeline is not initialized");

            for (auto imageView: m_shadowMapImageViews) {
                vkDestroyImageView(m_engine->m_device, imageView, nullptr);
            }

            vkDestroyPipeline(m_engine->m_device, m_pipeline, nullptr);
            vkDestroyPipelineLayout(m_engine->m_device, m_pipelineLayout, nullptr);

            m_engine = nullptr;
            m_initialized = false;
        }
    }// namespace Pipeline
}// namespace moe