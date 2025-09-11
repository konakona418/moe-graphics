#include "Render/Vulkan/VulkanInitializers.hpp"

namespace moe {
    namespace VkInit {
        VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
            VkCommandPoolCreateInfo commandPoolCreateInfo{};
            commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolCreateInfo.pNext = nullptr;
            commandPoolCreateInfo.flags = flags;
            commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;

            return commandPoolCreateInfo;
        }

        VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count) {
            VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
            commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAllocateInfo.pNext = nullptr;
            commandBufferAllocateInfo.commandPool = pool;
            commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocateInfo.commandBufferCount = count;

            return commandBufferAllocateInfo;
        }


        VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags) {
            VkFenceCreateInfo fenceCreateInfo{};
            fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCreateInfo.pNext = nullptr;
            fenceCreateInfo.flags = flags;

            return fenceCreateInfo;
        }

        VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags) {
            VkSemaphoreCreateInfo semaphoreCreateInfo{};
            semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semaphoreCreateInfo.pNext = nullptr;
            semaphoreCreateInfo.flags = flags;

            return semaphoreCreateInfo;
        }

        VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags) {
            VkCommandBufferBeginInfo commandBufferBeginInfo{};
            commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            commandBufferBeginInfo.pNext = nullptr;

            commandBufferBeginInfo.flags = flags;
            commandBufferBeginInfo.pInheritanceInfo = nullptr;

            return commandBufferBeginInfo;
        }

        VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkSemaphore semaphore, VkPipelineStageFlags2 stageMask) {
            VkSemaphoreSubmitInfo semaphoreSubmitInfo{};
            semaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            semaphoreSubmitInfo.pNext = nullptr;

            semaphoreSubmitInfo.semaphore = semaphore;
            semaphoreSubmitInfo.stageMask = stageMask;
            semaphoreSubmitInfo.deviceIndex = 0;
            semaphoreSubmitInfo.value = 1;

            return semaphoreSubmitInfo;
        }

        VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer commandBuffer) {
            VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
            commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
            commandBufferSubmitInfo.pNext = nullptr;

            commandBufferSubmitInfo.commandBuffer = commandBuffer;
            commandBufferSubmitInfo.deviceMask = 0;

            return commandBufferSubmitInfo;
        }

        VkSubmitInfo2 submitInfo(VkCommandBufferSubmitInfo* commandBuffer, VkSemaphoreSubmitInfo* waitSemaphore, VkSemaphoreSubmitInfo* signalSemaphore) {
            VkSubmitInfo2 submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
            submitInfo.pNext = nullptr;

            submitInfo.pCommandBufferInfos = commandBuffer;
            submitInfo.commandBufferInfoCount = 1;

            submitInfo.pWaitSemaphoreInfos = waitSemaphore;
            submitInfo.waitSemaphoreInfoCount = waitSemaphore ? 1 : 0;

            submitInfo.pSignalSemaphoreInfos = signalSemaphore;
            submitInfo.signalSemaphoreInfoCount = signalSemaphore ? 1 : 0;

            return submitInfo;
        }

        VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usage, VkExtent3D extent) {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.pNext = nullptr;

            imageInfo.imageType = VK_IMAGE_TYPE_2D;

            imageInfo.format = format;
            imageInfo.extent = extent;
            imageInfo.usage = usage;

            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;

            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

            return imageInfo;
        }

        VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspects) {
            VkImageViewCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.pNext = nullptr;

            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.image = image;
            info.format = format;
            info.subresourceRange.aspectMask = aspects;

            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.layerCount = 1;

            return info;
        }
    }// namespace VkInit
}// namespace moe