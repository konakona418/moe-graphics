#pragma once
#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    using BaseIdType = size_t;

    using MaterialId = BaseIdType;
    using MeshId = BaseIdType;
    using ImageId = BaseIdType;

    constexpr MaterialId NULL_MATERIAL_ID = std::numeric_limits<MaterialId>::max();
    constexpr MeshId NULL_MESH_ID = std::numeric_limits<MeshId>::max();
    constexpr ImageId NULL_IMAGE_ID = std::numeric_limits<ImageId>::max();
}// namespace moe