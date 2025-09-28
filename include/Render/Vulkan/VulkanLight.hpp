#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    constexpr uint32_t MAX_LIGHTS = 1024;

    constexpr uint32_t LIGHT_TYPE_POINT = 0;
    constexpr uint32_t LIGHT_TYPE_DIRECTIONAL = 1;
    constexpr uint32_t LIGHT_TYPE_SPOT = 2;

    enum class LightType : uint32_t {
        Point = LIGHT_TYPE_POINT,
        Directional = LIGHT_TYPE_DIRECTIONAL,
        Spot = LIGHT_TYPE_SPOT,
    };

    struct VulkanGPULight {
        glm::vec3 position;// 12
        uint32_t type;     // 4

        glm::vec3 color;// 12
        float intensity;// 4

        glm::vec3 direction;// 12
        float radius;       // 4

        glm::vec3 spotParams;// 12 (x = lightAngleScale, y = lightAngleOffset, z = unused)
        uint32_t available;  // 4
    };

    struct VulkanCPULight {
        glm::vec3 position; // shared
        glm::vec3 color;    // shared
        glm::vec3 direction;// for directional and spot light
        LightType type;     // shared
        float radius;       // for point and spot light
        float intensity;    // shared

        // the inner cone angle and outer cone angle are in radians
        // as specified in glTF 2.0
        // spec: https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual#spot
        float innerConeAngleRad;// for spot light
        float outerConeAngleRad;// for spot light

        VulkanCPULight() = default;

        static VulkanCPULight createPointLight(const glm::vec3& pos, const glm::vec3& color, float intensity, float radius) {
            VulkanCPULight light{};
            light.type = LightType::Point;
            light.position = pos;
            light.color = color;
            light.intensity = intensity;
            light.radius = radius;
            return light;
        }

        static VulkanCPULight createDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity) {
            VulkanCPULight light{};
            light.type = LightType::Directional;
            light.direction = direction;
            light.color = color;
            light.intensity = intensity;
            return light;
        }

        static VulkanCPULight createSpotLight(
                const glm::vec3& position, const glm::vec3& direction,
                const glm::vec3& color,
                float intensity,
                float radius,
                float innerConeAngleRad, float outerConeAngleRad) {
            VulkanCPULight light{};
            light.type = LightType::Spot;
            light.position = position;
            light.direction = direction;
            light.color = color;
            light.intensity = intensity;
            light.radius = radius;
            light.innerConeAngleRad = innerConeAngleRad;
            light.outerConeAngleRad = outerConeAngleRad;
            return light;
        }

        VulkanGPULight toGPU() const {
            VulkanGPULight gpuLight{};
            gpuLight.position = position;
            gpuLight.type = static_cast<uint32_t>(type);

            gpuLight.color = color;
            gpuLight.intensity = intensity;

            gpuLight.direction = direction;
            gpuLight.radius = radius;

            gpuLight.spotParams = cvtConeAngleToSpotParams(innerConeAngleRad, outerConeAngleRad);
            gpuLight.available = 1;
            return gpuLight;
        }

    private:
        static glm::vec3 cvtConeAngleToSpotParams(float innerAngle, float outerAngle) {
            // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual#inner-and-outer-cone-angles
            float lightAngleScale =
                    1.0f / std::max(0.001f, std::cos(innerAngle) - std::cos(outerAngle));
            float lightAngleOffset = -std::cos(outerAngle) * lightAngleScale;
            return glm::vec3(lightAngleScale, lightAngleOffset, 0.0f);
        }
    };
}// namespace moe