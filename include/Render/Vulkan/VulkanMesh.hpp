#pragma once

#include "Render/Vulkan/VulkanSkeleton.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


namespace moe {
    struct VulkanMeshGeoSurface {
        uint32_t beginIndex;
        uint32_t indexCount;
    };

    struct VulkanGPUMeshBuffer {
        VulkanAllocatedBuffer vertexBuffer;
        VulkanAllocatedBuffer indexBuffer;
        VulkanAllocatedBuffer skinningDataBuffer;

        VkDeviceAddress vertexBufferAddr;
        VkDeviceAddress skinningDataBufferAddr;

        uint32_t indexCount;
        uint32_t vertexCount;

        bool hasSkinningData;
    };

    struct VulkanCPUMesh {
        Vector<Vertex> vertices;
        Vector<uint32_t> indices;
        Vector<SkinningData> skinningData;
        bool hasSkeleton{false};

        glm::vec3 min;
        glm::vec3 max;

        bool isDataDiscarded() const { return discarded; }

        void discard() {
            vertices.clear();
            vertices.shrink_to_fit();
            indices.clear();
            indices.shrink_to_fit();
            discarded = true;
        }

    private:
        bool discarded{false};
    };

    struct VulkanGPUMesh {
        VulkanGPUMeshBuffer gpuBuffer;

        glm::vec3 min;
        glm::vec3 max;
    };
}// namespace moe