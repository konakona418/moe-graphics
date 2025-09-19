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

    struct VulkanCPUMesh {
        Vector<Vertex> vertices;
        Vector<uint32_t> indices;

        glm::vec3 min;
        glm::vec3 max;
    };

    struct VulkanGPUMesh {
        VulkanGPUMeshBuffer gpuBuffer;

        glm::vec3 min;
        glm::vec3 max;
    };
}// namespace moe