#pragma once

#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    struct VulkanGPUMeshBuffer {
        VulkanAllocatedBuffer vertexBuffer;
        VulkanAllocatedBuffer indexBuffer;

        VkDeviceAddress vertexBufferAddr;

        uint32_t indexCount;
        uint32_t vertexCount;
    };
}// namespace moe