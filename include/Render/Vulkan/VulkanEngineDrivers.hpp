#pragma once

#include "Render/Vulkan/VulkanLight.hpp"
#include "Render/Vulkan/VulkanSwapBuffer.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    class VulkanEngine;

    struct VulkanIlluminationBus {
    public:
        enum class RegistryType {
            Static,
            Dynamic
        };

        void init(VulkanEngine& engine);

        void destroy();

        VulkanIlluminationBus& addLight(const VulkanCPULight& light, RegistryType type);

        VulkanIlluminationBus& addStaticLight(const VulkanCPULight& light) {
            return addLight(light, RegistryType::Static);
        }

        VulkanIlluminationBus& addDynamicLight(const VulkanCPULight& light) {
            return addLight(light, RegistryType::Dynamic);
        }

        VulkanIlluminationBus& setAmbient(const glm::vec3& color, float strength) {
            ambientColorStrength = glm::vec4(color, strength);
            return *this;
        }

        VulkanIlluminationBus& setSunlight(const glm::vec3& direction, const glm::vec3& color, float strength);

        VulkanSwapBuffer& getGPUBuffer() { return lightBuffer; }

        VulkanCPULight getSunlight() const { return sunLight; }

        glm::vec4 getAmbientColorStrength() const { return ambientColorStrength; }

        void uploadToGPU(VkCommandBuffer cmdBuffer, uint32_t frameIndex);

        // includes the sun light + static lights + dynamic lights
        size_t getNumLights() const { return staticLights.size() + dynamicLights.size() + 1; }

        void resetDynamicState() { dynamicLights.clear(); }

        void resetAllState() {
            staticLights.clear();
            dynamicLights.clear();
        }

    private:
        VulkanEngine* m_engine{nullptr};
        glm::vec4 ambientColorStrength{1.0f, 1.0f, 1.0f, 1.0f};// rgb color, a strength
        VulkanCPULight sunLight;
        glm::vec3 sunLightDirection;

        Vector<VulkanCPULight> staticLights;
        Vector<VulkanCPULight> dynamicLights;

        VulkanSwapBuffer lightBuffer;
    };

    struct VulkanSceneBus {
    public:
    private:
    };
}// namespace moe
