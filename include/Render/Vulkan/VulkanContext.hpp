#pragma once

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace moe {

    class VulkanContext {
    public:
        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

        VulkanContext() = default;
        ~VulkanContext() = default;

        void initialize(GLFWwindow* window);

        void shutdown();

        vkb::Instance m_instance;

        vkb::PhysicalDevice m_physicalDevice;
        vkb::Device m_device;

        VkSurfaceKHR m_surface;

        VkQueue m_graphicsQueue;
        VkQueue m_presentQueue;

    private:
        void createInstance();

        void lookupPhysicalDevice();

        void createSurface(GLFWwindow* window);

        void createDevice();

        void createQueues();
    };

}// namespace moe