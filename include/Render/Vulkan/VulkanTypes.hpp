#pragma once

#include "Core/Common.hpp"

#include <filesystem>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "Math/Common.hpp"

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

        VkDeviceAddress address{0};
    };

    struct Vertex {
        glm::vec3 pos;
        float uv_x;
        glm::vec3 normal;
        float uv_y;
        glm::vec4 tangent;
    };

    struct VulkanGPUSceneData {
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 viewProjection;

        glm::mat4 invView;
        glm::mat4 invProjection;
        glm::mat4 invViewProjection;

        glm::vec4 cameraPosition;

        glm::vec4 ambientColor;// color(3), intensity(1)
        glm::vec4 sunlightDirection;
        glm::vec4 sunlightColor;

        VkDeviceAddress materialBuffer;
        VkDeviceAddress lightBuffer;
        uint32_t numLights;

        uint32_t skyboxId;

#define MOE_USE_SHADOW_MAP
#ifdef MOE_USE_SHADOW_MAP
#define MOE_USE_CSM
#ifdef MOE_USE_CSM
        uint32_t csmShadowMapId;
        glm::mat4 csmShadowMapLightTransform[4];
        glm::vec4 shadowMapCascadeSplits;
#else
        uint32_t shadowMapId;
        glm::mat4 shadowMapLightTransform;
#endif
#endif
    };

}// namespace moe

#include <fmt/format.h>

#define MOE_VK_CHECK(expr) MOE_VK_CHECK_MSG(expr, "Vulkan error")
