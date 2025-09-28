#include "Render/Vulkan/VulkanEngineDrivers.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"

namespace moe {
    constexpr uint32_t BUS_LIGHT_WARNING_LIMIT = 128;

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
}// namespace moe