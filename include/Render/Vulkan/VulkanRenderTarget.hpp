#pragma once

#include <vulkan/vulkan.h>

namespace moe {
    class VulkanRenderTarget {
    public:
        virtual ~VulkanRenderTarget() = default;

        virtual VkExtent2D getExtent() = 0;

        virtual VkFramebuffer getFramebuffer() = 0;

        virtual VkRenderPass getRenderPass() = 0;

    private:
    };
}// namespace moe