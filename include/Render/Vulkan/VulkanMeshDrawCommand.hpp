#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"


namespace moe {
    struct VulkanMeshDrawCommand {
        MeshId meshId;
        glm::mat4 transform;

        MaterialId overrideMaterial{NULL_MATERIAL_ID};
    };
}// namespace moe