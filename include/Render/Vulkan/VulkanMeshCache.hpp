#pragma once

#include "Render/Vulkan/VulkanCacheUtils.hpp"
#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


// fwd decl
namespace moe {
    class VulkanEngine;
    class VulkanMeshAsset;
    class VulkanGPUMeshBufferSet;
}// namespace moe

namespace moe {
    struct VulkanMeshCache {
    public:
        VulkanMeshCache() = default;
        ~VulkanMeshCache() = default;

        void init(VulkanEngine& engine);

        MeshId loadMesh(VulkanMeshAsset mesh);

        MeshId loadMeshFromFile(StringView filename);

        void destroy();

    private:
        bool m_initialized{false};
        VulkanEngine* m_engine{nullptr};

        UnorderedMap<MeshId, VulkanMeshAsset> m_meshes;
        VulkanCacheIdAllocator<MeshId> m_idAllocator;
    };
}// namespace moe