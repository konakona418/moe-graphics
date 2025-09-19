#pragma once

#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    struct VulkanCamera {
        glm::vec3 pos{0.0f};

        float pitch{0.0f};
        float yaw{0.0f};

        glm::mat4 viewMatrix() const;
        glm::mat4 projectionMatrix(float aspectRatio) const;
    };
}// namespace moe