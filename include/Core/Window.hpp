#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <functional>
#include <string>

namespace moe {
    class Window {
    public:
        using ResizeCallbackPfn = std::function<void(int, int)>;

        Window() = default;
        ~Window() = default;

        void initialize(int width, int height, const std::string& title);

        void shutdown();

        GLFWwindow* getHandle() const { return m_window; }

        bool shouldClose() const { return glfwWindowShouldClose(m_window); }

        void setResizeCallback(const ResizeCallbackPfn& callback) { m_resizeCallback = callback; }

    private:
        GLFWwindow* m_window = nullptr;

        ResizeCallbackPfn m_resizeCallback;
    };
}// namespace moe