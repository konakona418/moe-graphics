#include "Render/Vulkan/Pipeline/MeshPipeline.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"
#include "Render/Vulkan/VulkanInitializers.hpp"
#include "Render/Vulkan/VulkanMeshCache.hpp"
#include "Render/Vulkan/VulkanPipeline.hpp"
#include "Render/Vulkan/VulkanUtils.hpp"


namespace moe {
    namespace Pipeline {
        void VulkanMeshPipeline::init(VulkanEngine& engine) {
            MOE_ASSERT(!m_initialized, "VulkanMeshPipeline already initialized");

            m_engine = &engine;
            m_initialized = true;

            auto vert =
                    VkUtils::createShaderModuleFromFile(engine.m_device, "shaders/mesh.vert.spv");
            auto frag =
                    VkUtils::createShaderModuleFromFile(engine.m_device, "shaders/mesh.frag.spv");

            auto pushRange = VkPushConstantRange{
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(PushConstants),
            };

            auto pushRanges =
                    Array<VkPushConstantRange, 1>{pushRange};

            VulkanDescriptorLayoutBuilder layoutBuilder{};
            // ! todo: layouts

            m_descriptorSetLayout =
                    layoutBuilder.build(
                            engine.m_device,
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

            auto descriptorLayouts =
                    Array<VkDescriptorSetLayout, 1>{m_descriptorSetLayout};

            // ! fixme: use bindless descriptor set
            m_descriptorSet =
                    m_engine->m_globalDescriptorAllocator.allocate(engine.m_device, descriptorLayouts[0]);

            auto pipelineLayoutInfo =
                    VkInit::pipelineLayoutCreateInfo(descriptorLayouts, pushRanges);

            MOE_VK_CHECK(vkCreatePipelineLayout(engine.m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

            m_pipeline =
                    VulkanPipelineBuilder(m_pipelineLayout)
                            .addShader(vert, frag)
                            .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                            .setPolygonMode(VK_POLYGON_MODE_FILL)
                            .setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
                            .disableMultisampling()
                            .disableBlending()
                            .enableDepthTesting(true, VK_COMPARE_OP_LESS)
                            .setColorAttachmentFormat(engine.m_drawImage.imageFormat)
                            .setDepthFormat(engine.m_depthImage.imageFormat)
                            .build(engine.m_device);

            vkDestroyShaderModule(engine.m_device, vert, nullptr);
            vkDestroyShaderModule(engine.m_device, frag, nullptr);
        }

        void VulkanMeshPipeline::draw(
                VkCommandBuffer cmdBuffer,
                VulkanMeshCache& meshCache,
                VulkanMaterialCache& materialCache,
                Span<VulkanMeshDrawCommand> drawCommands,
                VulkanAllocatedBuffer& sceneDataBuffer) {
            MOE_ASSERT(m_initialized, "VulkanMeshPipeline not initialized");

            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
            vkCmdBindDescriptorSets(
                    cmdBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_pipelineLayout,
                    0, 1, &m_descriptorSet,
                    0, nullptr);

            for (auto& cmd: drawCommands) {
                auto mesh = meshCache.getMesh(cmd.meshId);
                if (!mesh.has_value()) {
                    Logger::warn("Invalid mesh id {}, skipping draw command", cmd.meshId);
                    continue;
                }

                auto& meshAsset = mesh.value();
                for (auto& submesh: meshAsset.meshes) {
                    MOE_ASSERT(sceneDataBuffer.address != 0, "Invalid scene data buffer");

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

                    vkCmdBindIndexBuffer(cmdBuffer, submesh->gpuBuffer.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                    const auto pushConstants = PushConstants{
                            .transform = cmd.transform,
                            .vertexBufferAddr = submesh->gpuBuffer.vertexBufferAddr,
                            .sceneDataAddress = sceneDataBuffer.address,
                            .materialId = cmd.overrideMaterial,
                    };

                    vkCmdPushConstants(
                            cmdBuffer,
                            m_pipelineLayout,
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                            0,
                            sizeof(PushConstants),
                            &pushConstants);

                    vkCmdDrawIndexed(cmdBuffer, submesh->gpuBuffer.indexCount, 1, 0, 0, 0);
                }
            }
        }

        void VulkanMeshPipeline::destroy() {
            MOE_ASSERT(m_initialized, "VulkanMeshPipeline not initialized");

            vkDestroyDescriptorSetLayout(m_engine->m_device, m_descriptorSetLayout, nullptr);
            vkDestroyPipeline(m_engine->m_device, m_pipeline, nullptr);
            vkDestroyPipelineLayout(m_engine->m_device, m_pipelineLayout, nullptr);

            m_initialized = false;
            m_engine = nullptr;
        }
    }// namespace Pipeline
}// namespace moe