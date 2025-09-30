#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanRenderable.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


namespace moe {
    struct VulkanRenderNode : public VulkanRenderable {
        VulkanRenderNode* parent{nullptr};
        Vector<UniquePtr<VulkanRenderNode>> children;

        glm::mat4 localTransform{1.0f};
        glm::mat4 worldTransform{1.0f};

        void updateTransform(const glm::mat4& parentTransform) override;

        void gatherRenderPackets(Vector<VulkanRenderPacket>& packets, const VulkanDrawContext& drawContext) override = 0;
    };

    struct VulkanSceneNode : public VulkanRenderNode {
        SceneResourceInternalId resourceInternalId{NULL_SCENE_RESOURCE_INTERNAL_ID};// -> VulkanScene::meshes
        uint32_t sortKey{VulkanRenderPacket::INVALID_SORT_KEY};

        void gatherRenderPackets(Vector<VulkanRenderPacket>& packets, const VulkanDrawContext& drawContext) override;
    };

    struct VulkanSceneMesh {
        Vector<MeshId> primitives;
        Vector<MaterialId> primitiveMaterials;
    };

    struct VulkanScene : public VulkanRenderNode {
        Vector<VulkanSceneMesh> meshes;

        void gatherRenderPackets(Vector<VulkanRenderPacket>& packets) {
            VulkanDrawContext drawContext{.lastContext = nullptr, .sceneMeshes = &meshes};
            gatherRenderPackets(packets, drawContext);
        }

        void gatherRenderPackets(Vector<VulkanRenderPacket>& packets, const VulkanDrawContext& drawContext) override;
    };
}// namespace moe
