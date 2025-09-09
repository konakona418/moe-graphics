#include "Core/Logger.hpp"
#include "Render/Vulkan/VulkanRenderServer.hpp"

int main() {
    moe::Logger::info("Hello World!");
    moe::VulkanRenderServer renderServer;
    renderServer.initialize();
    renderServer.shutdown();
    return 0;
}