#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    namespace VkUtils {
        constexpr uint64_t secsToNanoSecs(float secs) { return static_cast<uint64_t>(secs * 1000000000.0f); }

        void transitionImage(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout);

        VkImageSubresourceRange makeImageSubresourceRange(VkImageAspectFlags aspectFlags);

        void copyImage(VkCommandBuffer cmdBuffer, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize);
    }// namespace VkUtils
}// namespace moe