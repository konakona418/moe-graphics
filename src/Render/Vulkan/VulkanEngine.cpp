#include "Render/Vulkan/VulkanEngine.hpp"
#include "Render/Vulkan/VulkanInitializers.hpp"
#include "Render/Vulkan/VulkanLoaders.hpp"
#include "Render/Vulkan/VulkanMesh.hpp"
#include "Render/Vulkan/VulkanUtils.hpp"


#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <chrono>
#include <thread>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"


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
        initDescriptors();

        initBindlessSet();
        initCaches();

        initPipelines();

        initImGUI();

        m_isInitialized = true;
    }

    void VulkanEngine::run() {
        bool shouldQuit = false;

        std::pair<uint32_t, uint32_t> newMetric;

        while (!shouldQuit) {
            glfwPollEvents();

            while (auto e = pollEvent()) {
                if (e->is<WindowEvent::Close>()) {
                    Logger::debug("window closing...");
                    shouldQuit = true;
                }

                if (e->is<WindowEvent::Minimize>()) {
                    Logger::debug("window minimized");
                    m_stopRendering = true;
                } else if (e->is<WindowEvent::RestoreFromMinimize>()) {
                    Logger::debug("window restored from minimize");
                    m_stopRendering = false;
                }

                if (auto resize = e->getIf<WindowEvent::Resize>()) {
                    if (resize->width && resize->height) {
                        // Logger::debug("window resize: {}x{}", resize->width, resize->height);
                        m_resizeRequested = true;
                        newMetric = {resize->width, resize->height};
                    }
                }
            }

            if (m_resizeRequested) {
                Logger::debug("resize args: {}x{}", newMetric.first, newMetric.second);
                recreateSwapchain(newMetric.first, newMetric.second);

                m_resizeRequested = false;
            }

            if (m_stopRendering) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::ShowDemoWindow();

            ImGui::Render();

            draw();
        }
    }

    void VulkanEngine::immediateSubmit(Function<void(VkCommandBuffer)>&& fn) {
        MOE_VK_CHECK(vkResetFences(m_device, 1, &m_immediateModeFence));
        MOE_VK_CHECK(vkResetCommandBuffer(m_immediateModeCommandBuffer, 0));

        auto commandBuffer = m_immediateModeCommandBuffer;
        VkCommandBufferBeginInfo beginInfo = VkInit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        MOE_VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        fn(commandBuffer);

        MOE_VK_CHECK(vkEndCommandBuffer(commandBuffer));

        auto commandSubmitInfo = VkInit::commandBufferSubmitInfo(commandBuffer);
        auto submitInfo = VkInit::submitInfo(&commandSubmitInfo, nullptr, nullptr);

        MOE_VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submitInfo, m_immediateModeFence));

        MOE_VK_CHECK(vkWaitForFences(m_device, 1, &m_immediateModeFence, VK_TRUE, UINT64_MAX));
    }

    VulkanAllocatedBuffer VulkanEngine::allocateBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;

        bufferInfo.size = size;
        bufferInfo.usage = usage;

        VmaAllocationCreateInfo vmaAllocInfo{};
        vmaAllocInfo.usage = memoryUsage;
        vmaAllocInfo.flags =
                VMA_ALLOCATION_CREATE_MAPPED_BIT |
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        // todo: allow random access

        VulkanAllocatedBuffer buffer;
        MOE_VK_CHECK(
                vmaCreateBuffer(
                        m_allocator,
                        &bufferInfo,
                        &vmaAllocInfo,
                        &buffer.buffer,
                        &buffer.vmaAllocation,
                        &buffer.vmaAllocationInfo));

        if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
            VkBufferDeviceAddressInfo deviceAddrInfo{};
            deviceAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            deviceAddrInfo.buffer = buffer.buffer;

            buffer.address = vkGetBufferDeviceAddress(m_device, &deviceAddrInfo);
        } else {
            buffer.address = 0;
        }

        return buffer;
    }

    VulkanAllocatedImage VulkanEngine::allocateImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmap) {
        VkImageCreateInfo imageInfo = VkInit::imageCreateInfo(format, usage, extent);

        if (mipmap) {
            imageInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
        } else {
            imageInfo.mipLevels = 1;
        }

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.flags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VulkanAllocatedImage image{};
        image.imageExtent = extent;
        image.imageFormat = format;

        MOE_VK_CHECK(
                vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &image.image, &image.vmaAllocation, nullptr));

        VkImageAspectFlags aspect =
                format == VK_FORMAT_D32_SFLOAT
                        ? VK_IMAGE_ASPECT_DEPTH_BIT
                        : VK_IMAGE_ASPECT_COLOR_BIT;
        VkImageViewCreateInfo imageViewInfo = VkInit::imageViewCreateInfo(format, image.image, aspect);
        imageViewInfo.subresourceRange.levelCount = imageInfo.mipLevels;

        MOE_VK_CHECK(vkCreateImageView(m_device, &imageViewInfo, nullptr, &image.imageView));

        return image;
    }

    VulkanAllocatedImage VulkanEngine::allocateImage(void* data, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmap) {
        size_t imageSize = extent.width * extent.height * extent.depth * 4;// rgba8
        VulkanAllocatedBuffer stagingBuffer = allocateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        std::memcpy(stagingBuffer.vmaAllocation->GetMappedData(), data, imageSize);

        VulkanAllocatedImage image = allocateImage(
                extent, format,
                usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                mipmap);
        immediateSubmit([&](VkCommandBuffer cmd) {
            VkUtils::transitionImage(
                    cmd, image.image,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBufferImageCopy copyRegion{};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;

            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.imageExtent = extent;

            vkCmdCopyBufferToImage(
                    cmd,
                    stagingBuffer.buffer,
                    image.image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &copyRegion);

            VkUtils::transitionImage(
                    cmd, image.image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

        destroyBuffer(stagingBuffer);

        return image;
    }

    Optional<VulkanAllocatedImage> VulkanEngine::loadImageFromFile(StringView filename, VkFormat format, VkImageUsageFlags usage, bool mipmap) {
        int width, height, channels;
        auto rawImage = VkLoaders::loadImage(filename, &width, &height, &channels);

        if (!rawImage) {
            Logger::error("Failed to open image: {}", filename);
            return {std::nullopt};
        }

        VulkanAllocatedImage image = allocateImage(
                rawImage.get(),
                {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
                format,
                usage,
                mipmap);

        return image;
    }

    void VulkanEngine::destroyImage(VulkanAllocatedImage& image) {
        vkDestroyImageView(m_device, image.imageView, nullptr);
        vmaDestroyImage(m_allocator, image.image, image.vmaAllocation);
    }

    void VulkanEngine::destroyBuffer(VulkanAllocatedBuffer& buffer) {
        vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.vmaAllocation);
    }

    VulkanGPUMeshBuffer VulkanEngine::uploadMesh(Span<uint32_t> indices, Span<Vertex> vertices) {
        const size_t vertBufferSize = vertices.size() * sizeof(Vertex);
        const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

        VulkanGPUMeshBuffer surface;
        surface.indexCount = static_cast<uint32_t>(indices.size());
        surface.vertexCount = static_cast<uint32_t>(vertices.size());

        surface.vertexBuffer = allocateBuffer(
                vertBufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);

        VkBufferDeviceAddressInfo deviceAddrInfo{};
        deviceAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        deviceAddrInfo.buffer = surface.vertexBuffer.buffer;

        surface.vertexBufferAddr = vkGetBufferDeviceAddress(m_device, &deviceAddrInfo);

        surface.indexBuffer = allocateBuffer(
                indexBufferSize,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);

        VulkanAllocatedBuffer stagingBuffer = allocateBuffer(
                vertBufferSize + indexBufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_ONLY);

        void* data = stagingBuffer.vmaAllocation->GetMappedData();

        std::memcpy(data, vertices.data(), vertBufferSize);
        std::memcpy(static_cast<uint8_t*>(data) + vertBufferSize,
                    indices.data(), indexBufferSize);

        immediateSubmit([&](VkCommandBuffer cmdBuffer) {
            VkBufferCopy vertCopy{};
            vertCopy.srcOffset = 0;
            vertCopy.dstOffset = 0;
            vertCopy.size = vertBufferSize;

            vkCmdCopyBuffer(cmdBuffer,
                            stagingBuffer.buffer, surface.vertexBuffer.buffer,
                            1, &vertCopy);

            VkBufferCopy indexCopy{};
            indexCopy.srcOffset = vertBufferSize;
            indexCopy.dstOffset = 0;
            indexCopy.size = indexBufferSize;

            vkCmdCopyBuffer(cmdBuffer,
                            stagingBuffer.buffer, surface.indexBuffer.buffer,
                            1, &indexCopy);
        });

        destroyBuffer(stagingBuffer);

        return surface;
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

        currentFrame.descriptorAllocator.clearPools(m_device);

        MOE_VK_CHECK_MSG(
                vkResetFences(m_device, 1, &currentFrame.inFlightFence),
                "Failed to reset fence");

        uint32_t swapchainImageIndex;
        VkResult acquireResult = vkAcquireNextImageKHR(
                m_device,
                m_swapchain,
                VkUtils::secsToNanoSecs(1.0f),
                currentFrame.imageAvailableSemaphore,
                VK_NULL_HANDLE,
                &swapchainImageIndex);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
            // ! note that with resize event hooked, no need to set resize flag here.
            // ! another note: some drivers can fix suboptimal cases automatically, which is quite absurd.
            Logger::warn("vkAcquireNextImageKHR returned VK_ERROR_OUT_OF_DATE_KHR, forcing resize.");
            m_resizeRequested = true;
            return;
        } else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
            MOE_LOG_AND_THROW("Failed to acquire next swapchain image");
        }

        VkCommandBuffer commandBuffer = currentFrame.mainCommandBuffer;

        m_drawExtent.width = m_drawImage.imageExtent.width;
        m_drawExtent.height = m_drawImage.imageExtent.height;

        MOE_VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));

        VkCommandBufferBeginInfo beginInfo = VkInit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        MOE_VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        //! fixme: general image layout is not an optimal choice.
        VkUtils::transitionImage(
                commandBuffer, m_drawImage.image,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        VkUtils::transitionImage(
                commandBuffer, m_depthImage.image,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        //drawBackground(commandBuffer);

        VkClearValue clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}};
        VkClearValue depthClearValue = {.depthStencil = {1.0f, 0}};
        auto colorAttachment = VkInit::renderingAttachmentInfo(m_drawImage.imageView, &clearValue, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        auto depthAttachment = VkInit::renderingAttachmentInfo(m_depthImage.imageView, &depthClearValue, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        auto renderInfo = VkInit::renderingInfo(m_drawExtent, &colorAttachment, &depthAttachment);
        vkCmdBeginRendering(commandBuffer, &renderInfo);

        m_pipelines.testScene.updateTransform(glm::mat4(1.0f));

        Vector<VulkanRenderPacket> packets;
        m_pipelines.testScene.gatherRenderPackets(packets);

        Vector<VulkanMeshDrawCommand> drawCmds;
        drawCmds.reserve(packets.size());
        for (auto& packet: packets) {
            drawCmds.push_back(VulkanMeshDrawCommand{
                    .meshId = packet.meshId,
                    .transform = packet.transform,
                    .overrideMaterial = packet.materialId,
            });
        }

        m_pipelines.meshPipeline.draw(
                commandBuffer,
                m_caches.meshCache,
                m_caches.materialCache,
                drawCmds,
                m_pipelines.sceneDataBuffer);
        vkCmdEndRendering(commandBuffer);

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
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        drawImGUI(commandBuffer, m_swapchainImageViews[swapchainImageIndex]);

        VkUtils::transitionImage(
                commandBuffer, m_swapchainImages[swapchainImageIndex],
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

        VkResult presentResult = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
            // ! note that with resize event hooked, no need to set resize flag here.
            Logger::warn("vkQueuePresentKHR returned VK_ERROR_OUT_OF_DATE_KHR, forcing resize.");
            m_resizeRequested = true;
        }

        m_frameNumber++;
    }

    void VulkanEngine::drawBackground(VkCommandBuffer commandBuffer) {
        VkClearColorValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        VkImageSubresourceRange range = VkUtils::makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

        vkCmdClearColorImage(commandBuffer, m_drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &range);
    }

    void VulkanEngine::drawImGUI(VkCommandBuffer commandBuffer, VkImageView drawTarget) {
        auto attachment = VkInit::renderingAttachmentInfo(drawTarget, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        auto renderInfo = VkInit::renderingInfo(m_swapchainExtent, &attachment, nullptr);

        vkCmdBeginRendering(commandBuffer, &renderInfo);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        vkCmdEndRendering(commandBuffer);
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

        // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(m_windowExtent.width, m_windowExtent.height, "Moe Graphics Engine", nullptr, nullptr);

        if (!m_window) {
            MOE_LOG_AND_THROW("Fail to create GLFWwindow");
        }

        glfwSetWindowUserPointer(m_window, this);

        glfwSetWindowCloseCallback(m_window, [](auto* window) {
            auto* engine = static_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
            engine->queueEvent({WindowEvent::Close{}});
        });

        glfwSetWindowIconifyCallback(m_window, [](auto* window, int iconified) {
            auto* engine = static_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
            if (iconified) {
                engine->queueEvent({WindowEvent::Minimize{}});
            } else {
                engine->queueEvent({WindowEvent::RestoreFromMinimize{}});
            }
        });

        glfwSetWindowSizeCallback(m_window, [](auto* window, int width, int height) {
            auto* engine = static_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));

            engine->queueEvent({WindowEvent::Resize{
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)}});
        });
    }

    void VulkanEngine::initVulkanInstance() {
        vkb::InstanceBuilder builder;

        Logger::info("Initializing Vulkan instance...");
        Logger::info("Using Vulkan api version {}.{}.{}", 1, 3, 0);

        auto result =
                builder.set_app_name("Moe Graphics Engine")
                        .request_validation_layers(enableValidationLayers)
                        .add_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
                        .set_debug_messenger_type(VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
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
                                    } else {
                                        Logger::info("Validation Layer: {}", data->pMessage);
                                    }
                                    return VK_FALSE;
                                })
                        .require_api_version(1, 3, 0)
                        .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT)
                        .build();

        if (!result) {
            MOE_LOG_AND_THROW("Fail to create Vulkan instance");
        }

        auto vkbInstance = *result;

        m_instance = vkbInstance.instance;
        m_debugMessenger = vkbInstance.debug_messenger;

        Logger::info("Vulkan instance created.");

        MOE_VK_CHECK(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface));

        VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures = {
                .imageCubeArray = VK_TRUE,
                .geometryShader = VK_TRUE,
                .depthClamp = VK_TRUE,
                .samplerAnisotropy = VK_TRUE,
        };

        VkPhysicalDeviceVulkan13Features vkPhysicalDeviceVulkan13Features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                .synchronization2 = VK_TRUE,
                .dynamicRendering = VK_TRUE,
        };

        VkPhysicalDeviceVulkan12Features vkPhysicalDeviceVulkan12Features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
                .descriptorIndexing = VK_TRUE,
                // used for bindless descriptors
                .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
                .descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
                .descriptorBindingPartiallyBound = VK_TRUE,
                .descriptorBindingVariableDescriptorCount = VK_TRUE,
                .runtimeDescriptorArray = VK_TRUE,
                .scalarBlockLayout = VK_TRUE,
                .bufferDeviceAddress = VK_TRUE,
        };

        vkb::PhysicalDeviceSelector physicalDeviceSelector{vkbInstance};
        auto selectionResult =
                physicalDeviceSelector.set_minimum_version(1, 3)
                        .set_required_features(vkPhysicalDeviceFeatures)
                        .set_required_features_12(vkPhysicalDeviceVulkan12Features)
                        .set_required_features_13(vkPhysicalDeviceVulkan13Features)
                        .add_required_extension("VK_EXT_descriptor_indexing")
                        .add_required_extension("VK_KHR_shader_non_semantic_info")
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

    void VulkanEngine::initImGUI() {
        VkDescriptorPoolSize poolSizes[]{
                {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000;
        poolInfo.poolSizeCount = (uint32_t) std::size(poolSizes);
        poolInfo.pPoolSizes = poolSizes;

        VkDescriptorPool imguiPool;
        MOE_VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &imguiPool));

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& imGuiIo = ImGui::GetIO();
        imGuiIo.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(m_window, true);

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = m_instance;
        initInfo.PhysicalDevice = m_physicalDevice;
        initInfo.Device = m_device;
        initInfo.Queue = m_graphicsQueue;
        initInfo.DescriptorPool = imguiPool;
        initInfo.MinImageCount = FRAMES_IN_FLIGHT;
        initInfo.ImageCount = FRAMES_IN_FLIGHT;
        initInfo.UseDynamicRendering = true;

        initInfo.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_swapchainImageFormat;

        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&initInfo);

        m_mainDeletionQueue.pushFunction([=]() {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();

            vkDestroyDescriptorPool(m_device, imguiPool, nullptr);
        });
    }

    void VulkanEngine::initSwapchain() {
        createSwapchain(m_windowExtent.width, m_windowExtent.height);

        VkExtent3D drawExtent = {
                m_windowExtent.width,
                m_windowExtent.height,
                1};

        m_drawExtent = {drawExtent.width, drawExtent.height};

        m_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        m_drawImage.imageExtent = drawExtent;

        m_depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
        m_depthImage.imageExtent = drawExtent;

        VkImageUsageFlags drawImageUsage{};
        drawImageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        drawImageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        drawImageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        drawImageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;

        VkImageUsageFlags depthImageUsage{};
        depthImageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        VkImageCreateInfo drawImageInfo = VkInit::imageCreateInfo(m_drawImage.imageFormat, drawImageUsage, drawExtent);

        VkImageCreateInfo depthImageInfo = VkInit::imageCreateInfo(m_depthImage.imageFormat, depthImageUsage, drawExtent);

        VmaAllocationCreateInfo allocCreateInfo{};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocCreateInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vmaCreateImage(m_allocator, &drawImageInfo, &allocCreateInfo, &m_drawImage.image, &m_drawImage.vmaAllocation, nullptr);
        vmaCreateImage(m_allocator, &depthImageInfo, &allocCreateInfo, &m_depthImage.image, &m_depthImage.vmaAllocation, nullptr);

        VkImageViewCreateInfo drawImageViewInfo = VkInit::imageViewCreateInfo(m_drawImage.imageFormat, m_drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageViewCreateInfo depthImageViewInfo = VkInit::imageViewCreateInfo(m_depthImage.imageFormat, m_depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

        MOE_VK_CHECK(vkCreateImageView(m_device, &drawImageViewInfo, nullptr, &m_drawImage.imageView));
        MOE_VK_CHECK(vkCreateImageView(m_device, &depthImageViewInfo, nullptr, &m_depthImage.imageView));

        m_mainDeletionQueue.pushFunction([=] {
            vkDestroyImageView(m_device, m_drawImage.imageView, nullptr);
            vmaDestroyImage(m_allocator, m_drawImage.image, m_drawImage.vmaAllocation);

            vkDestroyImageView(m_device, m_depthImage.imageView, nullptr);
            vmaDestroyImage(m_allocator, m_depthImage.image, m_depthImage.vmaAllocation);
        });
    }

    void VulkanEngine::createSwapchain(uint32_t width, uint32_t height) {
        vkb::SwapchainBuilder swapchainBuilder{m_physicalDevice, m_device, m_surface};

        m_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

        auto swapchainResult =
                swapchainBuilder.set_desired_format({m_swapchainImageFormat, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
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

    void VulkanEngine::recreateSwapchain(uint32_t width, uint32_t height) {
        vkDeviceWaitIdle(m_device);

        destroySwapchain();

        m_windowExtent.width = width;
        m_windowExtent.height = height;

        createSwapchain(m_windowExtent.width, m_windowExtent.height);
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

        // ImGUI
        MOE_VK_CHECK(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_immediateModeCommandPool));

        auto allocInfo = VkInit::commandBufferAllocateInfo(m_immediateModeCommandPool);
        MOE_VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, &m_immediateModeCommandBuffer));

        m_mainDeletionQueue.pushFunction([=] {
            vkDestroyCommandPool(m_device, m_immediateModeCommandPool, nullptr);
        });
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

        MOE_VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &m_immediateModeFence));
        m_mainDeletionQueue.pushFunction([=] {
            vkDestroyFence(m_device, m_immediateModeFence, nullptr);
        });
    }

    void VulkanEngine::initCaches() {
        m_caches.imageCache.init(*this);
        m_caches.meshCache.init(*this);
        m_caches.materialCache.init(*this);

        m_mainDeletionQueue.pushFunction([&] {
            m_caches.materialCache.destroy();
            m_caches.meshCache.destroy();
            m_caches.imageCache.destroy();
        });
    }

    void VulkanEngine::initDescriptors() {
        Vector<VulkanDescriptorAllocator::PoolSizeRatio> ratios = {
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
        };

        m_globalDescriptorAllocator.init(m_device, 10, ratios);

        // !
        // todo

        Vector<VulkanDescriptorAllocatorDynamic::PoolSizeRatio> dynamicRatios = {
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
        };

        for (int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
            m_frames[i].descriptorAllocator = {};
            m_frames[i].descriptorAllocator.init(m_device, 10, dynamicRatios);

            // ! note that the index 'i' should not be captured by reference here
            m_mainDeletionQueue.pushFunction([&, i] {
                m_frames[i].descriptorAllocator.destroyPools(m_device);
            });
        }

        m_mainDeletionQueue.pushFunction([&] {
            m_globalDescriptorAllocator.destroyPool(m_device);
        });
    }

    void VulkanEngine::initBindlessSet() {
        m_bindlessSet.init(*this);
        m_mainDeletionQueue.pushFunction([&] {
            m_bindlessSet.destroy();
        });
    }

    void VulkanEngine::initPipelines() {
        m_pipelines.meshPipeline.init(*this);

        m_pipelines.sceneDataBuffer =
                allocateBuffer(
                        sizeof(VulkanGPUSceneData),
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        VMA_MEMORY_USAGE_AUTO);

        auto* sceneData = static_cast<VulkanGPUSceneData*>(m_pipelines.sceneDataBuffer.vmaAllocation->GetMappedData());
        sceneData->view = glm::lookAt(
                glm::vec3(-1.f, 2.f, 1.f),
                glm::vec3(0.f, 1.f, 0.f),
                glm::vec3(0.f, 1.f, 0.f));
        sceneData->projection = glm::perspective(
                glm::radians(45.f),
                (float) m_drawExtent.width / (float) m_drawExtent.height,
                0.1f, 10000.f);
        sceneData->projection[1][1] *= -1;
        sceneData->viewProjection = sceneData->projection * sceneData->view;
        sceneData->materialBuffer = m_caches.materialCache.getMaterialBufferAddress();

        m_pipelines.testScene = *VkLoaders::GLTF::loadSceneFromFile(*this, "./bz_v1/bz_v1.gltf");

        m_mainDeletionQueue.pushFunction([&] {
            destroyBuffer(m_pipelines.sceneDataBuffer);

            m_pipelines.meshPipeline.destroy();
        });
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