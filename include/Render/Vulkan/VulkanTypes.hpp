#pragma once

#include "Core/Common.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <glm/mat4x4.hpp>
#include <glm/packing.hpp>
#include <glm/vec4.hpp>

namespace moe {

    struct VulkanAllocatedImage {
        VkImage image;
        VkImageView imageView;
        VmaAllocation vmaAllocation;
        VkExtent3D imageExtent;
        VkFormat imageFormat;
    };

    struct VulkanAllocatedBuffer {
        VkBuffer buffer;
        VmaAllocation vmaAllocation;
        VmaAllocationInfo vmaAllocationInfo;
    };

    struct Vertex {
        glm::vec3 pos;
        float uv_x;
        glm::vec3 normal;
        float uv_y;
        glm::vec4 color;
    };

    struct VulkanGPUDrawPushConstants {
        glm::mat4 transform;
        VkDeviceAddress vertexBufferAddr;
    };

    struct VulkanGPUSceneData {
        glm::vec4 ambientColor;
        glm::vec4 sunlightDirection;
        glm::vec4 sunlightColor;
    };

}// namespace moe

#include <fmt/format.h>

#include "Core/Logger.hpp"

#define MOE_ASSERT(_cond, _msg) (assert(_cond&& _msg))

#define MOE_LOG_AND_THROW(_msg) \
    Logger::critical(_msg);     \
    throw std::runtime_error(_msg);

#define MOE_VK_CHECK_MSG(expr, msg)                     \
    do {                                                \
        VkResult result = expr;                         \
        if (result != VK_SUCCESS) {                     \
            Logger::critical(msg);                      \
            throw std::runtime_error(std::string(msg)); \
        }                                               \
    } while (false)

#define MOE_VK_CHECK(expr) MOE_VK_CHECK_MSG(expr, "Vulkan error")
