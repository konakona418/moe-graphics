#pragma once

#include "Render/Vulkan/VulkanGPUMesh.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    class VulkanEngine;

    namespace VkLoaders {
        Optional<VulkanMeshAsset> loadGLTFMeshFromFile(VulkanEngine& engine, StringView filename);
    }// namespace VkLoaders
}// namespace moe