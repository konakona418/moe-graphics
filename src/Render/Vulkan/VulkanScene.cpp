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

                if (drawContext.jointMatrixStartIndex != INVALID_JOINT_MATRIX_START_INDEX) {
                    packet.skinned = true;
                    packet.jointMatrixStartIndex = drawContext.jointMatrixStartIndex;
                } else {
                    packet.skinned = false;
                    packet.jointMatrixStartIndex = INVALID_JOINT_MATRIX_START_INDEX;
                }

                packets.push_back(packet);
            }
        }

        VulkanDrawContext localCtx{
                .lastContext = &drawContext,
                .sceneMeshes = drawContext.sceneMeshes,
                .jointMatrixStartIndex = drawContext.jointMatrixStartIndex,
        };

        packets.reserve(packets.size() + children.size());
        for (auto& child: children) {
            child->gatherRenderPackets(packets, localCtx);
        }
    }

    void VulkanScene::gatherRenderPackets(Vector<VulkanRenderPacket>& packets, const VulkanDrawContext& drawContext) {
        if (drawContext.isNull()) {
            VulkanDrawContext localCtx{
                    .lastContext = nullptr,
                    .sceneMeshes = &meshes,
                    .jointMatrixStartIndex = drawContext.jointMatrixStartIndex,
            };
            gatherRenderPackets(packets, localCtx);
            return;
        }

        VulkanDrawContext localCtx{
                .lastContext = &drawContext,
                .sceneMeshes = &meshes,
                .jointMatrixStartIndex = drawContext.jointMatrixStartIndex,
        };

        packets.reserve(packets.size() + children.size());

        for (auto& child: children) {
            child->gatherRenderPackets(packets, drawContext);
        }
    }
}// namespace moe