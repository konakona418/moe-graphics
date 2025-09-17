#include "Render/Vulkan/VulkanUtils.hpp"

#include <fstream>

namespace moe {
    namespace VkUtils {
        void transitionImage(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout) {
            VkImageMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.pNext = nullptr;

            //! fixme: this is not an optimal practice
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;

            barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

            barrier.oldLayout = srcLayout;
            barrier.newLayout = dstLayout;

            VkImageAspectFlags aspect =
                    (dstLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                            ? VK_IMAGE_ASPECT_DEPTH_BIT
                            : VK_IMAGE_ASPECT_COLOR_BIT;

            barrier.subresourceRange = makeImageSubresourceRange(aspect);
            barrier.image = image;

            VkDependencyInfo dependencyInfo{};
            dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependencyInfo.pNext = nullptr;

            dependencyInfo.imageMemoryBarrierCount = 1;
            dependencyInfo.pImageMemoryBarriers = &barrier;

            vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
        }

        VkImageSubresourceRange makeImageSubresourceRange(VkImageAspectFlags aspectFlags) {
            VkImageSubresourceRange range{};
            range.aspectMask = aspectFlags;
            range.baseMipLevel = 0;
            range.levelCount = VK_REMAINING_MIP_LEVELS;
            range.baseArrayLayer = 0;
            range.layerCount = VK_REMAINING_ARRAY_LAYERS;

            return range;
        }

        void copyImage(VkCommandBuffer cmdBuffer, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize) {
            VkImageBlit2 blitRgn{};
            blitRgn.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
            blitRgn.pNext = nullptr;

            blitRgn.srcOffsets[1].x = srcSize.width;
            blitRgn.srcOffsets[1].y = srcSize.height;
            blitRgn.srcOffsets[1].z = 1;

            blitRgn.dstOffsets[1].x = dstSize.width;
            blitRgn.dstOffsets[1].y = dstSize.height;
            blitRgn.dstOffsets[1].z = 1;

            blitRgn.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRgn.srcSubresource.baseArrayLayer = 0;
            blitRgn.srcSubresource.layerCount = 1;
            blitRgn.srcSubresource.mipLevel = 0;

            blitRgn.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRgn.dstSubresource.baseArrayLayer = 0;
            blitRgn.dstSubresource.layerCount = 1;
            blitRgn.dstSubresource.mipLevel = 0;

            VkBlitImageInfo2 blitInfo{};
            blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
            blitInfo.pNext = nullptr;

            blitInfo.srcImage = src;
            blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

            blitInfo.dstImage = dst;
            blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            blitInfo.regionCount = 1;
            blitInfo.pRegions = &blitRgn;

            blitInfo.filter = VK_FILTER_LINEAR;

            vkCmdBlitImage2(cmdBuffer, &blitInfo);
        }

        VkShaderModule createShaderModuleFromFile(VkDevice device, StringView filename) {
            std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                MOE_LOG_AND_THROW(
                        fmt::format("Failed to open shader file: {}", filename).c_str());
            }

            size_t fileSize = static_cast<size_t>(file.tellg());
            file.seekg(0);
            std::vector<char> buffer(fileSize);
            file.read(buffer.data(), fileSize);
            file.close();

            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.pNext = nullptr;

            createInfo.codeSize = buffer.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

            VkShaderModule shaderModule;
            MOE_VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

            return shaderModule;
        }
    }// namespace VkUtils
}// namespace moe