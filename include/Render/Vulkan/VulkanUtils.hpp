#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    namespace VkUtils {
        constexpr uint64_t secsToNanoSecs(float secs) { return static_cast<uint64_t>(secs * 1000000000.0f); }

        void transitionImage(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout);

        VkImageSubresourceRange makeImageSubresourceRange(VkImageAspectFlags aspectFlags);

        void copyImage(VkCommandBuffer cmdBuffer, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize);

        void copyFromBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkImage dstImage, VkExtent3D imageExtent, bool mipmap = false);

        void copyFromBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkImage dstImage, VkExtent3D imageExtent, VkOffset3D imageOffset, size_t bufferOffset, size_t bufferRowLength, size_t bufferImageHeight);

        void resolveImage(VkCommandBuffer cmdBuffer, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize);

        VkShaderModule createShaderModuleFromFile(VkDevice device, StringView filename);

        void generateMipmaps(VkCommandBuffer cmdBuffer, VkImage image, VkExtent2D imageExtent);

        size_t getBytesPerPixelFromFormat(VkFormat format);

        size_t getChannelsFromFormat(VkFormat format);
    }// namespace VkUtils
}// namespace moe