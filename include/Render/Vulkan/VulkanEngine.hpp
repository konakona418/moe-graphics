#pragma once

#include "Render/Vulkan/VulkanDescriptors.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

#include <GLFW/glfw3.h>

namespace moe {
    struct DeletionQueue {
        Deque<Function<void()>> deletors;

        void pushFunction(Function<void()>&& function);

        void flush();
    };

    struct FrameData {
        VkCommandPool commandPool;
        VkCommandBuffer mainCommandBuffer;

        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;

        VkFence inFlightFence;

        DeletionQueue deletionQueue;
    };

    constexpr uint32_t FRAMES_IN_FLIGHT = 2;

    class VulkanEngine {
    public:
        DeletionQueue m_mainDeletionQueue;
        VmaAllocator m_allocator;
        VulkanDescriptorAllocator m_globalDescriptorAllocator;

        bool m_isInitialized{false};
        int32_t m_frameNumber{0};
        bool m_stopRendering{false};
        VkExtent2D m_windowExtent{1280, 720};

        GLFWwindow* m_window{nullptr};

        VkInstance m_instance;
        VkDebugUtilsMessengerEXT m_debugMessenger;
        VkPhysicalDevice m_physicalDevice;
        VkDevice m_device;
        VkSurfaceKHR m_surface;

        VkSwapchainKHR m_swapchain;
        VkFormat m_swapchainImageFormat;
        Vector<VkImage> m_swapchainImages;
        Vector<VkImageView> m_swapchainImageViews;
        VkExtent2D m_swapchainExtent;

        Array<FrameData, FRAMES_IN_FLIGHT> m_frames;

        VkQueue m_graphicsQueue;
        uint32_t m_graphicsQueueFamilyIndex;

        VulkanAllocatedImage m_drawImage;
        VkExtent2D m_drawExtent;

        VkFence m_immediateModeFence;
        VkCommandBuffer m_immediateModeCommandBuffer;
        VkCommandPool m_immediateModeCommandPool;

        static VulkanEngine& get();

        void init();

        void cleanup();

        void draw();

        void run();

        void immediateSubmit(Function<void(VkCommandBuffer)>&& fn);

        FrameData& getCurrentFrame() { return m_frames[m_frameNumber % FRAMES_IN_FLIGHT]; }

    private:
        enum class WindowEventType : uint8_t {
            Close,
            Minimized,
            Restored,
        };

        struct WindowEvent {
            WindowEventType type;

            template<WindowEventType E>
            bool is() { return type == E; }
        };

        Queue<WindowEvent> m_pollingEvents;

        void initWindow();

        void queueEvent(WindowEvent event);

        Optional<WindowEvent> pollEvent();

        void initVulkanInstance();

        void initImGUI();

        void initSwapchain();

        void createSwapchain(uint32_t width, uint32_t height);

        void destroySwapchain();

        void initCommands();

        void initSyncPrimitives();

        void initDescriptors();

        void drawBackground(VkCommandBuffer commandBuffer);

        void drawImGUI(VkCommandBuffer commandBuffer, VkImageView drawTarget);
    };

}// namespace moe