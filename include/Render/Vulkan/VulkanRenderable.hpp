#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


namespace moe {
    struct VulkanSceneMesh;

    constexpr size_t INVALID_JOINT_MATRIX_START_INDEX = std::numeric_limits<size_t>::max();

    struct VulkanRenderPacket {
        static constexpr uint32_t INVALID_SORT_KEY = std::numeric_limits<uint32_t>::max();

        MeshId meshId;
        MaterialId materialId;
        glm::mat4 transform;
        uint32_t sortKey{INVALID_SORT_KEY};// todo: add sorting key

        bool skinned{false};
        size_t jointMatrixStartIndex{INVALID_JOINT_MATRIX_START_INDEX};// for skinned meshes
    };

    struct VulkanDrawContext {
        const VulkanDrawContext* lastContext{nullptr};
        Vector<VulkanSceneMesh>* sceneMeshes{nullptr};

        size_t jointMatrixStartIndex{INVALID_JOINT_MATRIX_START_INDEX};

        bool isNull() const { return lastContext == nullptr && sceneMeshes == nullptr; }
    };

    const static VulkanDrawContext NULL_DRAW_CONTEXT = {nullptr, nullptr, INVALID_JOINT_MATRIX_START_INDEX};

    struct VulkanRenderable {
        virtual ~VulkanRenderable() = default;

        virtual void updateTransform(const glm::mat4& parentTransform) = 0;
        virtual void gatherRenderPackets(Vector<VulkanRenderPacket>& packets, const VulkanDrawContext& drawContext) = 0;
    };
}// namespace moe
