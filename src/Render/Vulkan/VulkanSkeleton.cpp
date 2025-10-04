#include "Render/Vulkan/VulkanSkeleton.hpp"
#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>

namespace moe {
    namespace {
        struct SampleData {
            size_t prevIdx;
            size_t nextIdx;
            float interpolation;
        };

        constexpr uint32_t ANIMATION_FRAMES_PER_SECOND = 30;
    }// namespace

    SampleData calculateSample(size_t keysSize, float time) {
        SampleData sampleData;

        sampleData.prevIdx = std::min((size_t) (time * ANIMATION_FRAMES_PER_SECOND), keysSize - 1);
        sampleData.nextIdx = std::min(sampleData.prevIdx + 1, keysSize - 1);

        if (sampleData.prevIdx == sampleData.nextIdx) {
            sampleData.interpolation = 0.0f;
            return sampleData;
        }

        sampleData.interpolation = (time * ANIMATION_FRAMES_PER_SECOND) - sampleData.prevIdx;

        return sampleData;
    }

    glm::mat4 sampleTransform(const VulkanSkeletonAnimation& animation, JointId jointIndex, float time) {
        auto& track = animation.tracks[jointIndex];

        glm::mat4 translation{1.0f}, rotation{1.0f}, scale{1.0f};

        {
            auto& translations = track.translations;
            if (!translations.empty()) {
                auto sampleData = calculateSample(translations.size(), time);
                auto pos = glm::lerp(
                        translations[sampleData.prevIdx],
                        translations[sampleData.nextIdx],
                        sampleData.interpolation);
                translation = glm::translate(glm::mat4(1.0f), pos);
            }
        }

        {
            auto& rotations = track.rotations;
            if (!rotations.empty()) {
                auto sampleData = calculateSample(rotations.size(), time);
                auto rot = glm::slerp(
                        rotations[sampleData.prevIdx],
                        rotations[sampleData.nextIdx],
                        sampleData.interpolation);
                rotation = glm::mat4_cast(rot);
            }
        }

        {
            auto& scales = track.scales;
            if (!scales.empty()) {
                auto sampleData = calculateSample(scales.size(), time);
                auto sc = glm::lerp(
                        scales[sampleData.prevIdx],
                        scales[sampleData.nextIdx],
                        sampleData.interpolation);
                scale = glm::scale(glm::mat4(1.0f), sc);
            }
        }

        return translation * rotation * scale;
    }
    void calculateJointMatrixImpl(
            Vector<glm::mat4>& outJointMatrices,
            const VulkanSkeleton& skeleton,
            const VulkanSkeletonAnimation& animation,
            JointId jointIndex,
            glm::mat4 parentMatrix,
            float time) {
        auto local = sampleTransform(animation, jointIndex, time);
        auto model = parentMatrix * local;
        outJointMatrices[jointIndex] = model * skeleton.inverseBindMatrices[jointIndex];

        for (auto child: skeleton.hierarchy[jointIndex].children) {
            calculateJointMatrixImpl(outJointMatrices, skeleton, animation, child, model, time);
        }
    }

    void calculateJointMatrices(
            Vector<glm::mat4>& outJointMatrices,
            const VulkanSkeleton& skeleton,
            const VulkanSkeletonAnimation& animation,
            float time) {

        outJointMatrices.resize(skeleton.joints.size());
        calculateJointMatrixImpl(outJointMatrices, skeleton, animation, ROOT_JOINT_ID, glm::mat4(1.0f), time);
    }
}// namespace moe