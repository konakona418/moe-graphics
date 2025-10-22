#include "Render/Vulkan/VulkanEngineDrivers.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"

namespace moe {
    constexpr uint32_t BUS_LIGHT_WARNING_LIMIT = 128;

    RenderableId VulkanLoader::load(StringView path, Loader::GltfT) {
        MOE_ASSERT(m_engine, "VulkanLoader not initialized");
        return m_engine->m_caches.objectCache.load(m_engine, path, ObjectLoader::Gltf).first;
    }

    ImageId VulkanLoader::load(StringView path, Loader::ImageT) {
        MOE_ASSERT(m_engine, "VulkanLoader not initialized");
        return m_engine->m_caches.imageCache.loadImageFromFile(
                path,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT);
    }

    void VulkanIlluminationBus::init(VulkanEngine& engine) {
        m_engine = &engine;

        lightBuffer.init(
                engine,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                sizeof(VulkanGPULight) * MAX_LIGHTS,
                FRAMES_IN_FLIGHT);

        ambientColorStrength = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

        sunLight = VulkanCPULight::createDirectionalLight(glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), 1.0f);
        sunLightDirection = glm::vec3(0.0f, -1.0f, 0.0f);

        staticLights.clear();
        dynamicLights.clear();
    }

    void VulkanIlluminationBus::destroy() {
        lightBuffer.destroy();
        m_engine = nullptr;
        staticLights.clear();
        dynamicLights.clear();
    }

    VulkanIlluminationBus& VulkanIlluminationBus::addLight(const VulkanCPULight& light, RegistryType type) {
        if (light.type == LightType::Directional) {
            Logger::warn("Directional light cannot be added via VulkanIlluminationBus::addLight(), use setSunlight() instead");
            return *this;
        }

        if (type == RegistryType::Static) {
            staticLights.push_back(light);
        } else {
            dynamicLights.push_back(light);
        }
        return *this;
    }

    VulkanIlluminationBus& VulkanIlluminationBus::setSunlight(const glm::vec3& direction, const glm::vec3& color, float strength) {
        sunLight = VulkanCPULight::createDirectionalLight(direction, color, strength);
        sunLightDirection = direction;
        return *this;
    }

    void VulkanIlluminationBus::uploadToGPU(VkCommandBuffer cmdBuffer, uint32_t frameIndex) {
        Vector<VulkanGPULight> allLights;
        // ! fixme: suboptimal impl

        // sunlight + static lights + dynamic lights
        allLights.reserve(staticLights.size() + dynamicLights.size() + 1);

        allLights.push_back(sunLight.toGPU());
        for (const auto& light: staticLights) {
            allLights.push_back(light.toGPU());
        }
        for (const auto& light: dynamicLights) {
            allLights.push_back(light.toGPU());
        }

        if (allLights.size() > BUS_LIGHT_WARNING_LIMIT) {
            Logger::warn("Number of lights ({}) is growing too large (warning threshold {})! Check if the per-frame dynamic light cleanup is working correctly", allLights.size(), BUS_LIGHT_WARNING_LIMIT);
            allLights.resize(BUS_LIGHT_WARNING_LIMIT);
        }

        lightBuffer.upload(cmdBuffer, allLights.data(), frameIndex, sizeof(VulkanGPULight) * allLights.size());
    }

    VulkanRenderObjectBus& VulkanRenderObjectBus::submitSpriteRender(
            const ImageId imageId,
            const Transform& transform,
            const Color& color,
            const glm::vec2& size,
            const glm::vec2& texOffset,
            const glm::vec2& texSize) {
        MOE_ASSERT(m_initialized, "VulkanRenderObjectBus not initialized");
        if (m_spriteRenderCommands.size() >= MAX_SPRITE_RENDER_COMMANDS) {
            Logger::warn("Render object bus reached max sprite render commands(10240), check if dynamic state is reset properly; exceeding commands will be ignored");
            return *this;
        }

        auto realTexSize = texSize;
        if (texSize.x == 0.0f || texSize.y == 0.0f) {
            // if tex size is zero, use full size
            realTexSize = size;
        }

        m_spriteRenderCommands.push_back(VulkanSprite{
                .transform = transform,
                .color = color,
                .size = size,
                .texOffset = texOffset,
                .texSize = realTexSize,
                .textureId = imageId,
        });

        return *this;
    }
}// namespace moe