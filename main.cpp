#include "Render/Vulkan/VulkanEngine.hpp"

int main() {
    moe::VulkanEngine engine;
    engine.init();
    engine.run();
    engine.cleanup();
    return 0;
}