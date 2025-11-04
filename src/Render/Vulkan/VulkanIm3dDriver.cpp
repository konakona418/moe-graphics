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
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
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
            builder.setInputTopology(info.topology)
                    .setPolygonMode(VK_POLYGON_MODE_FILL)
                    .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                    .disableBlending()
                    .disableDepthTesting()
                    .setColorAttachmentFormat(drawImageFormat)
                    .disableMultisampling();

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
                MAX_VERTEX_COUNT * sizeof(Vertex),
                FRAMES_IN_FLIGHT);

        m_initialized = true;
    }

    void VulkanIm3dDriver::beginFrame(
            float deltaTime,
            glm::vec2 viewportSize,
            const VulkanCamera* camera,
            MouseState mouseState) {
        MOE_ASSERT(m_initialized, "VulkanIm3dDriver not initialized");

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

        uploadVertices(cmdBuffer);

        // todo: begin rendering
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
                    .size = dl.m_vertexCount * sizeof(Vertex),
                    .offset = vertexCountAccum * sizeof(Vertex),
            });

            vertexCountAccum += dl.m_vertexCount;
        }

        m_vertexBuffer.uploadMany(cmdBuffer, uploads, currentFrameIndex);
    }
}// namespace moe