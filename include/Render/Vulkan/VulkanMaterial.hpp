#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


namespace moe {
    struct VulkanCPUMaterial {
        glm::vec4 baseColor{glm::vec4(1.0f)};
        float metallic{0.0f};
        float roughness{1.0f};
        float emissive{0.0f};
        glm::vec4 emissiveColor{glm::vec4(1.0f)};

        ImageId diffuseTexture{NULL_IMAGE_ID};
        ImageId normalTexture{NULL_IMAGE_ID};
        ImageId metallicRoughnessTexture{NULL_IMAGE_ID};
        ImageId emissiveTexture{NULL_IMAGE_ID};

#ifdef MOE_DEBUG
        String name;
#endif
    };

    struct VulkanGPUMaterial {
        glm::vec4 baseColor;
        glm::vec4 metallicRoughnessEmissive;
        glm::vec4 emissiveColor;
        // .r = metallic, .g = roughness, .b = emmissive

        ImageId diffuseTexture;
        ImageId normalTexture;
        ImageId metallicRoughnessTexture;
        ImageId emissiveTexture;
    };

}// namespace moe