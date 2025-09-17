#pragma once

#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    struct VulkanMeshGeoSurface {
        uint32_t beginIndex;
        uint32_t indexCount;
    };

    struct VulkanGPUMeshBuffer {
        VulkanAllocatedBuffer vertexBuffer;
        VulkanAllocatedBuffer indexBuffer;

        VkDeviceAddress vertexBufferAddr;

        uint32_t indexCount;
        uint32_t vertexCount;
    };

    struct VulkanGPUMeshBufferSet {
        Vector<VulkanGPUMeshBuffer> buffers;
    };

    struct VulkanMesh {
        String name;
        Vector<VulkanMeshGeoSurface> surfaces;
        VulkanGPUMeshBuffer gpuBuffer;
    };

    struct VulkanMeshAsset {
        Vector<SharedPtr<VulkanMesh>> meshes;
    };
}// namespace moe