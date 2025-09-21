#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    constexpr uint32_t MAX_LIGHTS = 16;

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

        float radius;      // 4
        uint32_t available;// 4
        uint64_t _padding; // 8
    };

    struct VulkanCPULight {
        glm::vec3 position;
        glm::vec3 color;
        LightType type;
        float radius;
        float intensity;

        VulkanCPULight() = default;

        VulkanGPULight toGPU() const {
            VulkanGPULight gpuLight{};
            gpuLight.position = position;
            gpuLight.color = color;
            gpuLight.type = static_cast<uint32_t>(type);
            gpuLight.radius = radius;
            gpuLight.intensity = intensity;
            gpuLight.available = 1;
            return gpuLight;
        }
    };
}// namespace moe