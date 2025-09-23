#pragma once

#include "Render/Vulkan/Pipeline/MeshPipeline.hpp"
#include "Render/Vulkan/Pipeline/SkyBoxPipeline.hpp"
#include "Render/Vulkan/VulkanBindlessSet.hpp"
#include "Render/Vulkan/VulkanCamera.hpp"
#include "Render/Vulkan/VulkanDescriptors.hpp"
#include "Render/Vulkan/VulkanImageCache.hpp"
#include "Render/Vulkan/VulkanMaterialCache.hpp"
#include "Render/Vulkan/VulkanMeshCache.hpp"
#include "Render/Vulkan/VulkanScene.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


#include "Core/Input.hpp"


#include <GLFW/glfw3.h>

namespace moe {
    struct VulkanGPUMeshBuffer;

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
        VulkanDescriptorAllocatorDynamic descriptorAllocator;
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
        bool m_resizeRequested{false};
        VkExtent2D m_windowExtent{1280, 720};

        VkSampleCountFlagBits m_msaaSamples{VK_SAMPLE_COUNT_4_BIT};
        // ! msaa x4; disable this if deferred rendering is implemented; use fxaa then

        GLFWwindow* m_window{nullptr};
        std::pair<float, float> m_lastMousePos{0.0f, 0.0f};
        bool m_firstMouse{true};

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
        VulkanAllocatedImage m_depthImage;
        VulkanAllocatedImage m_msaaResolveImage;
        VkExtent2D m_drawExtent;

        VkFence m_immediateModeFence;
        VkCommandBuffer m_immediateModeCommandBuffer;
        VkCommandPool m_immediateModeCommandPool;

        VulkanBindlessSet m_bindlessSet;

        InputBus m_inputBus{};

        VulkanCamera m_defaultCamera{
                glm::vec3(0.0f, 0.0f, 0.0f),
                0.f,
                0.f,
                45.0f,
                0.1f,
                100.0f,
        };

        struct {
            VulkanImageCache imageCache;
            VulkanMeshCache meshCache;
            VulkanMaterialCache materialCache;
        } m_caches;

        struct {
            Pipeline::VulkanMeshPipeline meshPipeline;
            Pipeline::SkyBoxPipeline skyBoxPipeline;
            ImageId skyBoxImageId{NULL_IMAGE_ID};
            VulkanAllocatedBuffer lightBuffer;
            VulkanAllocatedBuffer sceneDataBuffer;
            VulkanScene testScene;
        } m_pipelines;

        static VulkanEngine& get();

        void init();

        void cleanup();

        void draw();

        void run();

        void beginFrame();

        void endFrame();

        VulkanBindlessSet& getBindlessSet() {
            MOE_ASSERT(m_bindlessSet.isInitialized(), "VulkanBindlessSet not initialized");
            return m_bindlessSet;
        }

        void immediateSubmit(Function<void(VkCommandBuffer)>&& fn);

        VulkanAllocatedBuffer allocateBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

        VulkanAllocatedImage allocateImage(VkImageCreateInfo imageInfo);

        VulkanAllocatedImage allocateImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmap = false);

        VulkanAllocatedImage allocateImage(void* data, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmap = false);

        VulkanAllocatedImage allocateCubeMapImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmap = false);

        VulkanAllocatedImage allocateCubeMapImage(Array<void*, 6> data, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmap = false);

        Optional<VulkanAllocatedImage> loadImageFromFile(StringView filename, VkFormat format, VkImageUsageFlags usage, bool mipmap = false);

        void destroyImage(VulkanAllocatedImage& image);

        void destroyBuffer(VulkanAllocatedBuffer& buffer);

        VulkanGPUMeshBuffer uploadMesh(Span<uint32_t> indices, Span<Vertex> vertices);

        FrameData& getCurrentFrame() { return m_frames[m_frameNumber % FRAMES_IN_FLIGHT]; }

        VulkanCamera& getDefaultCamera() { return m_defaultCamera; }

        InputBus& getInputBus() { return m_inputBus; }

        bool isMultisamplingEnabled() const { return m_msaaSamples != VK_SAMPLE_COUNT_1_BIT; }

    private:
        void initWindow();

        void queueEvent(WindowEvent event);

        void initVulkanInstance();

        void initImGUI();

        void initSwapchain();

        void createSwapchain(uint32_t width, uint32_t height);

        void destroySwapchain();

        void recreateSwapchain(uint32_t width, uint32_t height);

        void initCommands();

        void initSyncPrimitives();

        void initDescriptors();

        void initCaches();

        void initBindlessSet();

        void initPipelines();

        void drawBackground(VkCommandBuffer commandBuffer);

        void drawImGUI(VkCommandBuffer commandBuffer, VkImageView drawTarget);

        bool isKeyPressed(int key) const;
    };

}// namespace moe