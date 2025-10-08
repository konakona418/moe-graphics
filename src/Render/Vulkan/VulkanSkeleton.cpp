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

    }// namespace

    Pair<size_t, size_t> findDropInRange(Span<const float> arr, float value) {
        size_t n = arr.size();
        if (n == 0) {
            return {0, 0};
        }

        size_t left = 0;
        size_t right = n - 1;

        if (value <= arr[left]) {
            return {left, left};
        }
        if (value >= arr[right]) {
            return {right, right};
        }

        while (left <= right) {
            size_t mid = left + (right - left) / 2;
            if (arr[mid] == value) {
                return {mid, mid};
            } else if (arr[mid] < value) {
                left = mid + 1;
            } else {
                if (mid == 0) break;// prevent underflow
                right = mid - 1;
            }
        }

        size_t prevIdx = right < n ? right : n - 1;
        size_t nextIdx = left < n ? left : n - 1;

        return {prevIdx, nextIdx};
    }

    SampleData calculateSample(Span<const float> keyTimes, float time) {
        SampleData sampleData;

        // ! fixme: incorrect sampling:
        // !    - sampler type is forced to LINEAR

        if (keyTimes.size() <= 1) {
            return {0, 0, 0.0f};
        }

        auto [prevIdx, nextIdx] = findDropInRange(keyTimes, time);

        if (nextIdx == prevIdx) {
            return {prevIdx, nextIdx, 0.0f};
        }

        float t0 = keyTimes[prevIdx];
        float t1 = keyTimes[nextIdx];

        float interpolation = (time - t0) / (t1 - t0);

        MOE_ASSERT(prevIdx <= nextIdx, "Invalid key time indices");
        MOE_ASSERT(interpolation >= 0.0f && interpolation <= 1.0f, "Invalid interpolation factor");

        return {prevIdx, nextIdx, interpolation};
    }

    glm::mat4 sampleTransform(const VulkanSkeletonAnimation& animation, JointId jointIndex, float time) {
        auto& track = animation.tracks[jointIndex];

        glm::mat4 translation{1.0f}, rotation{1.0f}, scale{1.0f};

        {
            auto& translations = track.translations;
            if (!translations.empty()) {
                auto sampleData = calculateSample(track.keyTimes.translations, time);
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
                auto sampleData = calculateSample(track.keyTimes.rotations, time);
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
                auto sampleData = calculateSample(track.keyTimes.scales, time);
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