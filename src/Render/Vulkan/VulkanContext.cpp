#include "Render/Vulkan/VulkanContext.hpp"
#include "Core/Logger.hpp"


namespace moe {

    void VulkanContext::initialize(GLFWwindow* window) {
        Logger::info("Initializing Vulkan context...");

        createInstance();
        createSurface(window);
        lookupPhysicalDevice();
        createDevice();
        createQueues();

        Logger::info("Vulkan context initialized successfully.");
    }

    void VulkanContext::shutdown() {
        Logger::info("Shutting down Vulkan context...");
        vkb::destroy_device(m_device);

        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

        vkb::destroy_instance(m_instance);
        Logger::info("Vulkan context shut down successfully.");
    }


    void VulkanContext::createInstance() {
        vkb::InstanceBuilder instanceBuilder;
        instanceBuilder.set_app_name("Moe Graphics Engine")
                .require_api_version(1, 2, 0)
                .request_validation_layers()
                .use_default_debug_messenger()
                .set_debug_callback(
                        [](VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                           VkDebugUtilsMessageTypeFlagsEXT message_type,
                           const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                           void* user_data) -> VkBool32 {
                            if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                                if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
                                    Logger::error("Validation Layer: {}", callback_data->pMessage);
                                } else {
                                    Logger::warn("Validation Layer: {}", callback_data->pMessage);
                                }
                            }
                            return VK_FALSE;
                        });

        auto vkbInstance = instanceBuilder.build();

        if (!vkbInstance) {
            Logger::error("Failed to create Vulkan instance");
            throw std::runtime_error("Failed to create Vulkan instance");
        }

        m_instance = *vkbInstance;
    }

    void VulkanContext::createSurface(GLFWwindow* window) {
        if (!window) {
            Logger::critical("Null window handle passed to VulkanContext::createSurface");
            throw std::runtime_error("Null window handle");
        }
        if (glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface) != VK_SUCCESS) {
            Logger::critical("Failed to create Vulkan surface");
            throw std::runtime_error("Failed to create Vulkan surface");
        }
        Logger::info("Created Vulkan surface from window");
    }

    void VulkanContext::lookupPhysicalDevice() {
        vkb::PhysicalDeviceSelector selector{m_instance};
        auto requiredExtensions = std::vector<const char*>{
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        auto idealPhysicalDevice =
                selector
                        .add_required_extensions(requiredExtensions)
                        .set_minimum_version(1, 1)
                        .set_surface(m_surface)
                        .select();

        if (!idealPhysicalDevice) {
            Logger::critical("Failed to find a suitable GPU: {}", idealPhysicalDevice.error().message());
            throw std::runtime_error("Failed to find a suitable GPU");
        }

        m_physicalDevice = *idealPhysicalDevice;

        Logger::info("Using GPU: {}", idealPhysicalDevice->name);
    }

    void VulkanContext::createDevice() {
        vkb::DeviceBuilder device_builder{m_physicalDevice};
        auto vkbDevice = device_builder.build();

        if (!vkbDevice) {
            Logger::critical("Failed to create logical device: {}", vkbDevice.error().message());
            throw std::runtime_error("Failed to create logical device");
        }

        m_device = *vkbDevice;
        Logger::info("Vulkan device created successfully.");
    }

    void VulkanContext::createQueues() {
        auto graphicsQueue = m_device.get_queue(vkb::QueueType::graphics);
        if (!graphicsQueue) {
            Logger::critical("Failed to get graphics queue: {}", graphicsQueue.error().message());
            throw std::runtime_error("Failed to get graphics queue");
        }

        m_graphicsQueue = std::move(graphicsQueue.value());
        Logger::info("Graphics queue created successfully.");

        auto presentQueue = m_device.get_queue(vkb::QueueType::present);
        if (!presentQueue) {
            Logger::critical("Failed to get present queue: {}", presentQueue.error().message());
            throw std::runtime_error("Failed to get present queue");
        }

        m_presentQueue = std::move(presentQueue.value());
    }

}// namespace moe