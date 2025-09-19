#pragma once
#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    using BaseIdType = uint32_t;

    using MaterialId = BaseIdType;
    using MeshId = BaseIdType;
    using ImageId = BaseIdType;
    using SceneResourceInternalId = BaseIdType;

    constexpr MaterialId NULL_MATERIAL_ID = std::numeric_limits<MaterialId>::max();
    constexpr MeshId NULL_MESH_ID = std::numeric_limits<MeshId>::max();
    constexpr ImageId NULL_IMAGE_ID = std::numeric_limits<ImageId>::max();
    constexpr SceneResourceInternalId NULL_SCENE_RESOURCE_INTERNAL_ID = std::numeric_limits<SceneResourceInternalId>::max();

    namespace VkLoaders {
        namespace GLTF {
            using GLTFInternalId = size_t;
            constexpr GLTFInternalId NULL_GLTF_ID = std::numeric_limits<GLTFInternalId>::max();
        }// namespace GLTF
    }// namespace VkLoaders
}// namespace moe