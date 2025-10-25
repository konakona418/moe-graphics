#pragma once

#include "Math/Color.hpp"
#include "Math/Transform.hpp"

#include "Render/Vulkan/VulkanIdTypes.hpp"

namespace moe {
    struct VulkanSprite {
        Transform transform;
        Color color;

        glm::vec2 size{100.0f, 100.0f};

        glm::vec2 texOffset{0.0f, 0.0f};
        glm::vec2 texSize{0.0f, 0.0f};

        ImageId textureId{NULL_IMAGE_ID};

        bool isTextSprite{false};
    };
}// namespace moe