#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


namespace moe {
    struct VulkanSceneMesh;

    struct VulkanRenderPacket {
        static constexpr uint32_t INVALID_SORT_KEY = std::numeric_limits<uint32_t>::max();

        MeshId meshId;
        MaterialId materialId;
        glm::mat4 transform;
        uint32_t sortKey{INVALID_SORT_KEY};// todo: add sorting key
    };

    struct VulkanDrawContext {
        const VulkanDrawContext* lastContext{nullptr};
        Vector<VulkanSceneMesh>* sceneMeshes{nullptr};

        bool isNull() const { return lastContext == nullptr && sceneMeshes == nullptr; }
    };

    const static VulkanDrawContext NULL_DRAW_CONTEXT = {nullptr, nullptr};

    struct VulkanRenderable {
        virtual ~VulkanRenderable() = default;

        virtual void updateTransform(const glm::mat4& parentTransform) = 0;
        virtual void gatherRenderPackets(Vector<VulkanRenderPacket>& packets, const VulkanDrawContext& drawContext) = 0;
    };
}// namespace moe
