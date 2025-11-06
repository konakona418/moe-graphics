#include "Render/Vulkan/VulkanIm3dDriver.hpp"
#include "Render/Vulkan/VulkanCamera.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"
#include "Render/Vulkan/VulkanInitializers.hpp"
#include "Render/Vulkan/VulkanPipeline.hpp"
#include "Render/Vulkan/VulkanUtils.hpp"


namespace moe {
    namespace Im3dUtils {
        Im3d::Vec3 toIm3dVec3(const glm::vec3& v) {
            return Im3d::Vec3(v.x, v.y, v.z);
        }

        Im3d::Vec2 toIm3dVec2(const glm::vec2& v) {
            return Im3d::Vec2(v.x, v.y);
        }
    }// namespace Im3dUtils

    void VulkanIm3dDriver::init(
            VulkanEngine& engine,
            VkFormat drawImageFormat,
            VkFormat depthImageFormat) {
        MOE_ASSERT(!m_initialized, "VulkanIm3dDriver already initialized");

        m_engine = &engine;

        auto appData = Im3d::GetAppData();
        appData.m_flipGizmoWhenBehind = false;

        struct PerPipelineInfo {
            StringView vertShaderPath{};
            StringView fragShaderPath{};
            StringView geomShaderPath{};

            VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_MAX_ENUM};
            VkPipeline* outPipeline{nullptr};
            VkPipelineLayout* outPipelineLayout{nullptr};

            Function<void(VulkanPipelineBuilder*)> customizeBuilderFn{};
        };

        Array<PerPipelineInfo, 3> pipelineInfos;

        {
            auto& info = pipelineInfos[0];
            info.vertShaderPath = "shaders/im3d/point.vert.spv";
            info.fragShaderPath = "shaders/im3d/point.frag.spv";

            info.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

            info.outPipeline = &m_pipelines.pointPipeline;
            info.outPipelineLayout = &m_pipelines.pointPipelineLayout;
        }
        {
            auto& info = pipelineInfos[1];
            info.vertShaderPath = "shaders/im3d/line.vert.spv";
            info.fragShaderPath = "shaders/im3d/line.frag.spv";
            info.geomShaderPath = "shaders/im3d/line.geom.spv";

            info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

            info.outPipeline = &m_pipelines.linePipeline;
            info.outPipelineLayout = &m_pipelines.linePipelineLayout;
        }
        {
            auto& info = pipelineInfos[2];
            info.vertShaderPath = "shaders/im3d/triangle.vert.spv";
            info.fragShaderPath = "shaders/im3d/triangle.frag.spv";

            info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            info.outPipeline = &m_pipelines.trianglePipeline;
            info.outPipelineLayout = &m_pipelines.trianglePipelineLayout;
        }

        auto pushRange = VkPushConstantRange{
                .stageFlags =
                        VK_SHADER_STAGE_VERTEX_BIT |
                        VK_SHADER_STAGE_FRAGMENT_BIT |
                        VK_SHADER_STAGE_GEOMETRY_BIT,
                .offset = 0,
                .size = sizeof(PushConstants),
        };

        auto pushRanges =
                Array<VkPushConstantRange, 1>{pushRange};

        VulkanDescriptorLayoutBuilder layoutBuilder{};

        auto descriptorLayouts =
                Array<VkDescriptorSetLayout, 1>{
                        engine.getBindlessSet().getDescriptorSetLayout(),
                };

        auto pipelineLayoutInfo =
                VkInit::pipelineLayoutCreateInfo(descriptorLayouts, pushRanges);

        for (auto& info: pipelineInfos) {
            MOE_VK_CHECK(vkCreatePipelineLayout(engine.m_device, &pipelineLayoutInfo, nullptr, info.outPipelineLayout));

            auto vert =
                    VkUtils::createShaderModuleFromFile(engine.m_device, info.vertShaderPath);
            auto frag =
                    VkUtils::createShaderModuleFromFile(engine.m_device, info.fragShaderPath);

            VulkanPipelineBuilder builder{*info.outPipelineLayout};
            builder.setInputTopology(info.topology);
            builder.setPolygonMode(VK_POLYGON_MODE_FILL);
            builder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
            builder.enableBlending();
            builder.enableDepthTesting(VK_TRUE, VK_COMPARE_OP_LESS);
            builder.setColorAttachmentFormat(drawImageFormat);
            builder.setDepthFormat(depthImageFormat);
            builder.disableMultisampling();
            // no culling, enable blending, enable depth test, no msaa

            VkShaderModule geom = VK_NULL_HANDLE;
            if (!info.geomShaderPath.empty()) {
                geom = VkUtils::createShaderModuleFromFile(engine.m_device, info.geomShaderPath);
                builder.addShader(vert, frag, geom);
            } else {
                builder.addShader(vert, frag);
            }

            if (info.customizeBuilderFn) {
                info.customizeBuilderFn(&builder);
            }

            *info.outPipeline = builder.build(engine.m_device);

            vkDestroyShaderModule(engine.m_device, vert, nullptr);
            vkDestroyShaderModule(engine.m_device, frag, nullptr);
            if (geom != VK_NULL_HANDLE) {
                vkDestroyShaderModule(engine.m_device, geom, nullptr);
            }
        }

        m_vertexBuffer.init(
                engine,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                MAX_VERTEX_COUNT * sizeof(Im3d::VertexData),
                FRAMES_IN_FLIGHT);

        m_initialized = true;
    }

    void VulkanIm3dDriver::beginFrame(
            glm::vec2 viewportSize,
            const VulkanCamera* camera,
            MouseState mouseState) {
        MOE_ASSERT(m_initialized, "VulkanIm3dDriver not initialized");

        float deltaTime = 0.0f;
        {
            auto now = std::chrono::high_resolution_clock::now();
            if (m_islastUploadTimeValid) {
                deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(now - m_lastUploadTime).count();
            }
            m_lastUploadTime = now;
            m_islastUploadTimeValid = true;
        }

        Im3d::AppData& appData = Im3d::GetAppData();
        appData.m_deltaTime = deltaTime;
        appData.m_viewportSize = Im3dUtils::toIm3dVec2(viewportSize);
        appData.m_projOrtho = camera->isOrthographic();
        appData.m_viewOrigin = Im3dUtils::toIm3dVec3(camera->getPosition());
        appData.m_viewDirection = Im3dUtils::toIm3dVec3(camera->getFront());
        appData.m_worldUp = Im3dUtils::toIm3dVec3(Math::VK_GLOBAL_UP);
        if (camera->isOrthographic()) {
            appData.m_projScaleY = 1.0f;
        } else {
            auto fovYDeg = camera->getFovDeg();
            auto fovXDeg = fovYDeg * (viewportSize.x / viewportSize.y);
            appData.m_projScaleY = glm::tan(glm::radians(fovXDeg));
        }

        auto ndc =
                static_cast<glm::vec2>(mouseState.position) / static_cast<glm::vec2>(viewportSize) * 2.f -
                glm::vec2{1.f};
        const auto clip = glm::vec4{ndc, -1.f, 1.f};

        auto eye = glm::inverse(camera->projectionMatrix(viewportSize.x / viewportSize.y)) * clip;
        eye = glm::vec4{glm::vec2(eye), -1.f, 0.f};

        auto world = glm::inverse(camera->viewMatrix()) * eye;
        world = glm::normalize(world);

        glm::vec3 rayOrigin = camera->getPosition();
        glm::vec4 rayDirection = world;

        appData.m_cursorRayOrigin = Im3dUtils::toIm3dVec3(rayOrigin);
        appData.m_cursorRayDirection = Im3dUtils::toIm3dVec3(glm::vec3(rayDirection));

        appData.m_keyDown[Im3d::Mouse_Left] = mouseState.leftButtonDown;

        Im3d::NewFrame();
    }

    void VulkanIm3dDriver::endFrame() {
        MOE_ASSERT(m_initialized, "VulkanIm3dDriver not initialized");

        Im3d::EndFrame();
    }

    void VulkanIm3dDriver::render(
            VkCommandBuffer cmdBuffer,
            const VulkanCamera* camera,
            VkImageView targetImageView,
            VkImageView depthImageView,
            VkExtent2D extent) {
        MOE_ASSERT(m_initialized, "VulkanIm3dDriver not initialized");

        if (Im3d::GetDrawListCount() == 0) {
            return;
        }

        auto& appData = Im3d::GetAppData();

        uploadVertices(cmdBuffer);

        auto colorAttachment =
                VkInit::renderingAttachmentInfo(
                        targetImageView,
                        nullptr,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        auto depthAttachment =
                VkInit::renderingAttachmentInfo(
                        depthImageView,
                        nullptr,
                        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        auto renderInfo =
                VkInit::renderingInfo(
                        extent,
                        &colorAttachment,
                        &depthAttachment);

        vkCmdBeginRendering(cmdBuffer, &renderInfo);

        auto drawList = Im3d::GetDrawLists();
        Im3d::DrawPrimitiveType lastPrimitive = Im3d::DrawPrimitive_Count;

        size_t vertexOffset = 0;

        for (size_t i = 0; i < Im3d::GetDrawListCount(); i++) {
            auto& dl = drawList[i];
            if (dl.m_vertexCount == 0) {
                continue;
            }

            VkPipeline pipeline{VK_NULL_HANDLE};
            VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
            switch (dl.m_primType) {
                case Im3d::DrawPrimitive_Points:
                    pipeline = m_pipelines.pointPipeline;
                    pipelineLayout = m_pipelines.pointPipelineLayout;
                    break;
                case Im3d::DrawPrimitive_Lines:
                    pipeline = m_pipelines.linePipeline;
                    pipelineLayout = m_pipelines.linePipelineLayout;
                    break;
                case Im3d::DrawPrimitive_Triangles:
                    pipeline = m_pipelines.trianglePipeline;
                    pipelineLayout = m_pipelines.trianglePipelineLayout;
                    break;
                default:
                    MOE_ASSERT(false, "Unsupported Im3d primitive type");
            }

            if (dl.m_primType != lastPrimitive) {
                vkCmdBindPipeline(
                        cmdBuffer,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipeline);

                auto bindlessDescriptorSet =
                        m_engine->getBindlessSet().getDescriptorSet();
                vkCmdBindDescriptorSets(
                        cmdBuffer,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelineLayout,
                        0, 1, &bindlessDescriptorSet,
                        0, nullptr);

                lastPrimitive = dl.m_primType;

                auto viewport = VkViewport{
                        .x = 0,
                        .y = 0,
                        .width = (float) extent.width,
                        .height = (float) extent.height,
                        .minDepth = 0.f,
                        .maxDepth = 1.f,
                };
                vkCmdSetViewport(
                        cmdBuffer,
                        0, 1,
                        &viewport);

                auto scissor = VkRect2D{
                        .offset = {0, 0},
                        .extent = extent,
                };
                vkCmdSetScissor(
                        cmdBuffer,
                        0, 1,
                        &scissor);
            }

            auto viewport = glm::vec2{
                    appData.m_viewportSize.x,
                    appData.m_viewportSize.y,
            };

            glm::mat4 viewProj = camera->projectionMatrix(viewport.x / viewport.y) *
                                 camera->viewMatrix();

            auto pcs = PushConstants{
                    .vertexBufferAddr = m_vertexBuffer.getBuffer().address,
                    .viewProjection = viewProj,
                    .viewportSize = viewport,
            };

            vkCmdPushConstants(
                    cmdBuffer,
                    pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT |
                            VK_SHADER_STAGE_FRAGMENT_BIT |
                            VK_SHADER_STAGE_GEOMETRY_BIT,
                    0,
                    sizeof(PushConstants),
                    &pcs);

            vkCmdDraw(
                    cmdBuffer,
                    dl.m_vertexCount,
                    1,
                    vertexOffset,
                    0);

            vertexOffset += dl.m_vertexCount;
        }

        vkCmdEndRendering(cmdBuffer);
    }

    void VulkanIm3dDriver::destroy() {
        MOE_ASSERT(m_initialized, "VulkanIm3dDriver not initialized");

        vkDestroyPipeline(m_engine->m_device, m_pipelines.pointPipeline, nullptr);
        vkDestroyPipelineLayout(m_engine->m_device, m_pipelines.pointPipelineLayout, nullptr);

        vkDestroyPipeline(m_engine->m_device, m_pipelines.linePipeline, nullptr);
        vkDestroyPipelineLayout(m_engine->m_device, m_pipelines.linePipelineLayout, nullptr);

        vkDestroyPipeline(m_engine->m_device, m_pipelines.trianglePipeline, nullptr);
        vkDestroyPipelineLayout(m_engine->m_device, m_pipelines.trianglePipelineLayout, nullptr);

        m_vertexBuffer.destroy();

        m_engine = nullptr;
        m_initialized = false;
    }

    void VulkanIm3dDriver::uploadVertices(VkCommandBuffer cmdBuffer) {
        size_t currentFrameIndex = m_engine->getCurrentFrameIndex();

        size_t vertexCountAccum = 0;
        auto drawList = Im3d::GetDrawLists();

        Vector<VulkanSwapBuffer::UploadInfo> uploads;
        uploads.reserve(Im3d::GetDrawListCount());
        for (size_t i = 0; i < Im3d::GetDrawListCount(); i++) {
            auto& dl = drawList[i];
            if (vertexCountAccum + dl.m_vertexCount > MAX_VERTEX_COUNT) {
                Logger::warn("Im3d vertex count exceeds MAX_VERTEX_COUNT({}), exceeding vertices will be clamped", MAX_VERTEX_COUNT);
                break;
            }

            uploads.push_back({
                    .data = (void*) dl.m_vertexData,
                    .size = dl.m_vertexCount * sizeof(Im3d::VertexData),
                    .offset = vertexCountAccum * sizeof(Im3d::VertexData),
            });

            vertexCountAccum += dl.m_vertexCount;
        }

        m_vertexBuffer.uploadMany(cmdBuffer, uploads, currentFrameIndex);
    }
}// namespace moe