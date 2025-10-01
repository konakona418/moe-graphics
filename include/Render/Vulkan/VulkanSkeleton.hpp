#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    struct SkinningData {
        glm::vec<4, JointId> jointIds;
        glm::vec4 weights;
    };

    struct VulkanSkeleton {
        struct Joint {
            JointId id{NULL_JOINT_ID};
            glm::mat4 localTransform;
        };

        struct JointNode {
            Vector<JointId> children;
        };

        Vector<JointNode> hierarchy;
        Vector<glm::mat4> inverseBindMatrices;
        Vector<Joint> joints;

        Vector<String> jointNames;
    };
}// namespace moe