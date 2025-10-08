#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanRenderable.hpp"
#include "Render/Vulkan/VulkanSkeleton.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


namespace moe {
    struct VulkanRenderNode : public VulkanRenderable {
        VulkanRenderNode* parent{nullptr};
        Vector<UniquePtr<VulkanRenderNode>> children;

        glm::mat4 localTransform{1.0f};
        glm::mat4 worldTransform{1.0f};

        void updateTransform(const glm::mat4& parentTransform) override;

        void gatherRenderPackets(Vector<VulkanRenderPacket>& packets, const VulkanDrawContext& drawContext) override = 0;

        VulkanRenderableFeature getFeatures() const override { return VulkanRenderableFeature::HasHierarchy; }
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

    struct VulkanHasSkeletalAnimation {
        virtual const UnorderedMap<String, VulkanSkeletonAnimation>& getAnimations() const = 0;
        virtual const Vector<VulkanSkeleton>& getSkeletons() const = 0;
    };

    struct VulkanScene : public VulkanRenderNode, public VulkanHasSkeletalAnimation {
        Vector<VulkanSceneMesh> meshes;
        Vector<VulkanSkeleton> skeletons;
        UnorderedMap<String, VulkanSkeletonAnimation> animations;

        void gatherRenderPackets(Vector<VulkanRenderPacket>& packets) {
            VulkanDrawContext drawContext{
                    .lastContext = nullptr,
                    .sceneMeshes = &meshes,
                    .jointMatrixStartIndex = INVALID_JOINT_MATRIX_START_INDEX,
            };
            gatherRenderPackets(packets, drawContext);
        }

        void gatherRenderPackets(Vector<VulkanRenderPacket>& packets, const VulkanDrawContext& drawContext) override;

        VulkanRenderableFeature getFeatures() const override {
            Vector<VulkanRenderableFeature> features{VulkanRenderableFeature::HasHierarchy};
            if (!skeletons.empty() && !animations.empty()) {
                features.push_back(VulkanRenderableFeature::HasSkeletalAnimation);
            }
            return makeRenderFeatureBitmask(features);
        }

        const UnorderedMap<String, VulkanSkeletonAnimation>& getAnimations() const override { return animations; }

        const Vector<VulkanSkeleton>& getSkeletons() const override { return skeletons; }
    };
}// namespace moe
