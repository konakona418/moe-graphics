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

    struct VulkanSkeletonAnimation {
        struct Track {
            Vector<glm::vec3> translations;
            Vector<glm::quat> rotations;
            Vector<glm::vec3> scales;
        };

        Vector<Track> tracks;// index = jointId
        float duration;
        bool loop;

        size_t startFrame;

        String name;
    };

    void calculateJointMatrices(
            Vector<glm::mat4>& outJointMatrices,
            const VulkanSkeleton& skeleton,
            const VulkanSkeletonAnimation& animation,
            float time);
}// namespace moe