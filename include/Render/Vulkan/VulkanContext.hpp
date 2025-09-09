#pragma once

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace moe {

    class VulkanContext {
    public:
        VulkanContext() = default;
        ~VulkanContext() = default;

        void initialize();

        void shutdown();

    private:
        vkb::Instance m_instance;
        vkb::PhysicalDevice m_physicalDevice;
        vkb::Device m_device;

        GLFWwindow* m_window;
        VkSurfaceKHR m_surface;

        void createInstance();

        void createSurface();

        void lookupPhysicalDevice();

        void createDevice();
    };

}// namespace moe