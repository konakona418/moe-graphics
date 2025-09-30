#include "Render/Vulkan/VulkanScene.hpp"

namespace moe {
    void VulkanRenderNode::updateTransform(const glm::mat4& parentTransform) {
        worldTransform = parentTransform * localTransform;
        for (auto& child: children) {
            child->updateTransform(worldTransform);
        }
    }

    void VulkanSceneNode::gatherRenderPackets(Vector<VulkanRenderPacket>& packets, const VulkanDrawContext& drawContext) {
        MOE_ASSERT(drawContext.sceneMeshes != nullptr, "drawContext.sceneMeshes is null");

        if (resourceInternalId != NULL_SCENE_RESOURCE_INTERNAL_ID) {
            auto& sceneMesh = drawContext.sceneMeshes->at(resourceInternalId);

            for (int i = 0; i < sceneMesh.primitives.size(); ++i) {
                VulkanRenderPacket packet{
                        .meshId = sceneMesh.primitives[i],
                        .materialId = sceneMesh.primitiveMaterials[i],
                        .transform = worldTransform,
                        .sortKey = 0,
                };

                packets.push_back(packet);
            }
        }

        VulkanDrawContext localCtx{.lastContext = &drawContext, .sceneMeshes = drawContext.sceneMeshes};

        packets.reserve(packets.size() + children.size());
        for (auto& child: children) {
            child->gatherRenderPackets(packets, localCtx);
        }
    }

    void VulkanScene::gatherRenderPackets(Vector<VulkanRenderPacket>& packets, const VulkanDrawContext& drawContext) {
        if (drawContext.isNull()) {
            // dispatch to gather without context
            gatherRenderPackets(packets);
            return;
        }

        VulkanDrawContext localCtx{.lastContext = &drawContext, .sceneMeshes = &meshes};

        packets.reserve(packets.size() + children.size());

        for (auto& child: children) {
            child->gatherRenderPackets(packets, drawContext);
        }
    }
}// namespace moe