#include "Render/Vulkan/VulkanEngine.hpp"
#include "Render/Vulkan/VulkanInitializers.hpp"
#include "Render/Vulkan/VulkanUtils.hpp"

#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <chrono>
#include <thread>

namespace moe {
    constexpr bool enableValidationLayers{true};

    VulkanEngine* g_engineInstance{nullptr};

    void DeletionQueue::pushFunction(std::function<void()>&& function) {
        deletors.push_back(std::move(function));
    }

    void DeletionQueue::flush() {
        for (auto it = deletors.rbegin(); it != deletors.rend(); ++it) {
            (*it)();
        }
        deletors.clear();
    }

    VulkanEngine& VulkanEngine::get() {
        return *g_engineInstance;
    }

    void VulkanEngine::init() {
        MOE_ASSERT(g_engineInstance == nullptr, "engine instance already initialized");

        g_engineInstance = this;

        initWindow();
        initVulkanInstance();
        initSwapchain();
        initCommands();
        initSyncPrimitives();

        m_isInitialized = true;
    }

    void VulkanEngine::run() {
        bool shouldQuit = false;

        while (!shouldQuit) {
            glfwPollEvents();

            while (auto e = pollEvent()) {
                if (e->is<WindowEventType::Close>()) {
                    Logger::debug("window closing...");
                    shouldQuit = true;
                }

                if (e->is<WindowEventType::Minimized>()) {
                    Logger::debug("window minimized");
                    m_stopRendering = true;
                } else if (e->is<WindowEventType::Restored>()) {
                    Logger::debug("window restored from minimize");
                    m_stopRendering = false;
                }
            }

            if (m_stopRendering) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            draw();
        }
    }

    void VulkanEngine::draw() {
        auto& currentFrame = getCurrentFrame();

        MOE_VK_CHECK_MSG(
                vkWaitForFences(
                        m_device,
                        1, &currentFrame.inFlightFence,
                        VK_TRUE,
                        VkUtils::secsToNanoSecs(1.0f)),
                "Failed to wait for fence");

        currentFrame.deletionQueue.flush();

        MOE_VK_CHECK_MSG(
                vkResetFences(m_device, 1, &currentFrame.inFlightFence),
                "Failed to reset fence");

        uint32_t swapchainImageIndex;
        MOE_VK_CHECK_MSG(
                vkAcquireNextImageKHR(
                        m_device,
                        m_swapchain,
                        VkUtils::secsToNanoSecs(1.0f),
                        currentFrame.imageAvailableSemaphore,
                        VK_NULL_HANDLE,
                        &swapchainImageIndex),
                "Failed to acquire next image");

        VkCommandBuffer commandBuffer = currentFrame.mainCommandBuffer;

        m_drawExtent.width = m_drawImage.imageExtent.width;
        m_drawExtent.height = m_drawImage.imageExtent.height;

        MOE_VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));

        VkCommandBufferBeginInfo beginInfo = VkInit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        MOE_VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        //! fixme: general image layout is not an optimal choice.
        VkUtils::transitionImage(commandBuffer, m_drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        drawBackground(commandBuffer);

        VkUtils::transitionImage(
                commandBuffer, m_drawImage.image,
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        VkUtils::transitionImage(
                commandBuffer,
                m_swapchainImages[swapchainImageIndex],
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkUtils::copyImage(
                commandBuffer,
                m_drawImage.image, m_swapchainImages[swapchainImageIndex],
                m_drawExtent, m_swapchainExtent);

        VkUtils::transitionImage(
                commandBuffer, m_swapchainImages[swapchainImageIndex],
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        MOE_VK_CHECK(vkEndCommandBuffer(commandBuffer));

        VkCommandBufferSubmitInfo submitInfo = VkInit::commandBufferSubmitInfo(commandBuffer);
        VkSemaphoreSubmitInfo waitInfo =
                VkInit::semaphoreSubmitInfo(
                        currentFrame.imageAvailableSemaphore,
                        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR);
        VkSemaphoreSubmitInfo signalInfo =
                VkInit::semaphoreSubmitInfo(
                        currentFrame.renderFinishedSemaphore,
                        VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

        VkSubmitInfo2 submitInfo2 = VkInit::submitInfo(&submitInfo, &waitInfo, &signalInfo);
        MOE_VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submitInfo2, currentFrame.inFlightFence));

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;

        presentInfo.pSwapchains = &m_swapchain;
        presentInfo.swapchainCount = 1;

        presentInfo.pWaitSemaphores = &currentFrame.renderFinishedSemaphore;
        presentInfo.waitSemaphoreCount = 1;

        presentInfo.pImageIndices = &swapchainImageIndex;

        MOE_VK_CHECK(vkQueuePresentKHR(m_graphicsQueue, &presentInfo));

        m_frameNumber++;
    }

    void VulkanEngine::drawBackground(VkCommandBuffer commandBuffer) {
        VkClearColorValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        VkImageSubresourceRange range = VkUtils::makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

        vkCmdClearColorImage(commandBuffer, m_drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &range);
    }

    void VulkanEngine::cleanup() {
        if (m_isInitialized) {
            Logger::info("Cleaning up Vulkan engine...");

            vkDeviceWaitIdle(m_device);

            for (auto& frame: m_frames) {
                vkDestroyCommandPool(m_device, frame.commandPool, nullptr);

                vkDestroySemaphore(m_device, frame.imageAvailableSemaphore, nullptr);
                vkDestroySemaphore(m_device, frame.renderFinishedSemaphore, nullptr);
                vkDestroyFence(m_device, frame.inFlightFence, nullptr);

                frame.deletionQueue.flush();
            }

            m_mainDeletionQueue.flush();

            destroySwapchain();

            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

            vkDestroyDevice(m_device, nullptr);

            vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
            vkDestroyInstance(m_instance, nullptr);

            glfwDestroyWindow(m_window);
            m_window = nullptr;

            Logger::info("Vulkan engine cleaned up.");
        }

        g_engineInstance = nullptr;
    }

    void VulkanEngine::initWindow() {
        if (!glfwInit()) {
            MOE_LOG_AND_THROW("Fail to initialize glfw");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // todo: handle window resize.
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(m_windowExtent.width, m_windowExtent.height, "Moe Graphics Engine", nullptr, nullptr);

        if (!m_window) {
            MOE_LOG_AND_THROW("Fail to create GLFWwindow");
        }

        glfwSetWindowUserPointer(m_window, this);

        glfwSetWindowCloseCallback(m_window, [](auto* window) {
            auto* engine = static_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
            engine->queueEvent({WindowEventType::Close});
        });

        glfwSetWindowIconifyCallback(m_window, [](auto* window, int iconified) {
            auto* engine = static_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
            engine->queueEvent({iconified ? WindowEventType::Minimized : WindowEventType::Restored});
        });
    }

    void VulkanEngine::initVulkanInstance() {
        vkb::InstanceBuilder builder;

        Logger::info("Initializing Vulkan instance...");
        Logger::info("Using Vulkan api version {}.{}.{}", 1, 3, 0);

        auto result =
                builder.set_app_name("Moe Graphics Engine")
                        .request_validation_layers(enableValidationLayers)
                        .set_debug_callback(
                                [](VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                   VkDebugUtilsMessageTypeFlagsEXT type,
                                   const VkDebugUtilsMessengerCallbackDataEXT* data,
                                   void* ctx) -> VkBool32 {
                                    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                                        if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
                                            Logger::error("Validation Layer: {}", data->pMessage);
                                        } else {
                                            Logger::warn("Validation Layer: {}", data->pMessage);
                                        }
                                    }
                                    return VK_FALSE;
                                })
                        .require_api_version(1, 3, 0)
                        .build();

        if (!result) {
            MOE_LOG_AND_THROW("Fail to create Vulkan instance");
        }

        auto vkbInstance = *result;

        m_instance = vkbInstance.instance;
        m_debugMessenger = vkbInstance.debug_messenger;

        Logger::info("Vulkan instance created.");

        MOE_VK_CHECK(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface));

        VkPhysicalDeviceVulkan13Features vkPhysicalDeviceVulkan13Features{};
        vkPhysicalDeviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        vkPhysicalDeviceVulkan13Features.dynamicRendering = VK_TRUE;
        vkPhysicalDeviceVulkan13Features.synchronization2 = VK_TRUE;

        VkPhysicalDeviceVulkan12Features vkPhysicalDeviceVulkan12Features{};
        vkPhysicalDeviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        vkPhysicalDeviceVulkan12Features.bufferDeviceAddress = VK_TRUE;
        vkPhysicalDeviceVulkan12Features.descriptorIndexing = VK_TRUE;

        vkb::PhysicalDeviceSelector physicalDeviceSelector{vkbInstance};
        auto selectionResult =
                physicalDeviceSelector.set_minimum_version(1, 3)
                        .set_required_features_12(vkPhysicalDeviceVulkan12Features)
                        .set_required_features_13(vkPhysicalDeviceVulkan13Features)
                        .set_surface(m_surface)
                        .select();

        if (!selectionResult) {
            MOE_LOG_AND_THROW("Failed to select a valid physical device with proper features.");
        }

        auto vkbPhysicalDevice = *selectionResult;
        m_physicalDevice = vkbPhysicalDevice.physical_device;

        Logger::info("Using GPU: {}", vkbPhysicalDevice.properties.deviceName);

        vkb::DeviceBuilder deviceBuilder{vkbPhysicalDevice};
        auto deviceResult = deviceBuilder.build();

        if (!deviceResult) {
            MOE_LOG_AND_THROW("Failed to create a logical device.");
        }

        m_device = deviceResult->device;

        m_graphicsQueue = deviceResult->get_queue(vkb::QueueType::graphics).value();
        m_graphicsQueueFamilyIndex = deviceResult->get_queue_index(vkb::QueueType::graphics).value();

        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = m_physicalDevice;
        allocatorInfo.device = m_device;
        allocatorInfo.instance = m_instance;
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

        vmaCreateAllocator(&allocatorInfo, &m_allocator);

        m_mainDeletionQueue.pushFunction([&] {
            vmaDestroyAllocator(m_allocator);
        });
    }

    void VulkanEngine::initSwapchain() {
        createSwapchain(m_windowExtent.width, m_windowExtent.height);

        VkExtent3D drawExtent = {
                m_windowExtent.width,
                m_windowExtent.height,
                1};

        m_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        m_drawImage.imageExtent = drawExtent;

        VkImageUsageFlags usage{};
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        usage |= VK_IMAGE_USAGE_STORAGE_BIT;

        VkImageCreateInfo imageInfo = VkInit::imageCreateInfo(m_drawImage.imageFormat, usage, drawExtent);

        VmaAllocationCreateInfo allocCreateInfo{};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocCreateInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vmaCreateImage(m_allocator, &imageInfo, &allocCreateInfo, &m_drawImage.image, &m_drawImage.vmaAllocation, nullptr);

        VkImageViewCreateInfo imageViewInfo = VkInit::imageViewCreateInfo(m_drawImage.imageFormat, m_drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

        MOE_VK_CHECK(vkCreateImageView(m_device, &imageViewInfo, nullptr, &m_drawImage.imageView));

        m_mainDeletionQueue.pushFunction([=] {
            vkDestroyImageView(m_device, m_drawImage.imageView, nullptr);
            vmaDestroyImage(m_allocator, m_drawImage.image, m_drawImage.vmaAllocation);
        });
    }

    void VulkanEngine::createSwapchain(uint32_t width, uint32_t height) {
        vkb::SwapchainBuilder swapchainBuilder{m_physicalDevice, m_device, m_surface};

        m_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

        auto swapchainResult = swapchainBuilder.set_desired_format({m_swapchainImageFormat, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                       .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                       .set_desired_extent(width, height)
                                       .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                       .build();

        if (!swapchainResult) {
            MOE_LOG_AND_THROW("Failed to create swapchain.");
        }

        auto vkbSwapchain = swapchainResult.value();
        m_swapchain = vkbSwapchain.swapchain;
        m_swapchainExtent = vkbSwapchain.extent;
        m_swapchainImages = vkbSwapchain.get_images().value();
        m_swapchainImageViews = vkbSwapchain.get_image_views().value();
    }

    void VulkanEngine::destroySwapchain() {
        for (auto imageView: m_swapchainImageViews) {
            vkDestroyImageView(m_device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    }

    void VulkanEngine::initCommands() {
        auto createInfo =
                VkInit::commandPoolCreateInfo(
                        m_graphicsQueueFamilyIndex,
                        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        for (int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
            MOE_VK_CHECK_MSG(
                    vkCreateCommandPool(m_device, &createInfo, nullptr, &m_frames[i].commandPool),
                    "Failed to create command pool");

            auto allocInfo = VkInit::commandBufferAllocateInfo(m_frames[i].commandPool);
            MOE_VK_CHECK_MSG(
                    vkAllocateCommandBuffers(m_device, &allocInfo, &m_frames[i].mainCommandBuffer),
                    "Failed to allocate command buffer");
        }
    }

    void VulkanEngine::initSyncPrimitives() {
        VkFenceCreateInfo fenceInfo = VkInit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        VkSemaphoreCreateInfo semaphoreInfo = VkInit::semaphoreCreateInfo();

        for (int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
            MOE_VK_CHECK_MSG(
                    vkCreateFence(m_device, &fenceInfo, nullptr, &m_frames[i].inFlightFence),
                    "Failed to create fence");

            MOE_VK_CHECK_MSG(
                    vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_frames[i].imageAvailableSemaphore),
                    "Failed to create semaphore");

            MOE_VK_CHECK_MSG(
                    vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_frames[i].renderFinishedSemaphore),
                    "Failed to create semaphore");
        }
    }

    void VulkanEngine::queueEvent(WindowEvent event) {
        m_pollingEvents.push(event);
    }

    Optional<VulkanEngine::WindowEvent> VulkanEngine::pollEvent() {
        if (m_pollingEvents.empty()) {
            return {};
        }

        Optional<WindowEvent> ret = {m_pollingEvents.back()};
        m_pollingEvents.pop();
        return ret;
    }

}// namespace moe