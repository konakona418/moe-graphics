#pragma once
#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    using MaterialId = BaseIdType;
    using MeshId = BaseIdType;
    using SceneResourceInternalId = BaseIdType;
    using JointId = BaseIdType;

    constexpr MaterialId NULL_MATERIAL_ID = std::numeric_limits<MaterialId>::max();
    constexpr MeshId NULL_MESH_ID = std::numeric_limits<MeshId>::max();
    constexpr SceneResourceInternalId NULL_SCENE_RESOURCE_INTERNAL_ID = std::numeric_limits<SceneResourceInternalId>::max();
    constexpr JointId NULL_JOINT_ID = std::numeric_limits<JointId>::max();
    constexpr JointId ROOT_JOINT_ID = 0;

    namespace VkLoaders {
        namespace GLTF {
            using GLTFInternalId = size_t;
            constexpr GLTFInternalId NULL_GLTF_ID = std::numeric_limits<GLTFInternalId>::max();
        }// namespace GLTF
    }// namespace VkLoaders
}// namespace moe