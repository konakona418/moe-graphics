#pragma once

#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    namespace VkInit {
        VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);

        VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1);

        VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);

        VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

        VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);

        VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkSemaphore semaphore, VkPipelineStageFlags2 stageMask);

        VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer commandBuffer);

        VkSubmitInfo2 submitInfo(VkCommandBufferSubmitInfo* commandBuffer, VkSemaphoreSubmitInfo* waitSemaphore, VkSemaphoreSubmitInfo* signalSemaphore);

        VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usage, VkExtent3D extent);

        VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspects);

        VkRenderingAttachmentInfo renderingAttachmentInfo(VkImageView imageView, VkClearValue* clearValue, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkRenderingAttachmentInfo depthRenderingAttachmentInfo(VkImageView imageView, VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        VkRenderingInfo renderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment);

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo();

        VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module, const char* entryPoint = "main");

        VkPushConstantRange pushConstantRange(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset);
    }// namespace VkInit

}// namespace moe