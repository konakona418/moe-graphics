#include "Render/Vulkan/VulkanImageCache.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"


namespace moe {
    void VulkanImageCache::init(VulkanEngine& engine) {
        m_engine = &engine;
        m_initialized = true;
    }

    ImageId VulkanImageCache::addImage(VulkanAllocatedImage&& image) {
        MOE_ASSERT(m_initialized, "VulkanImageCache not initialized");

        ImageId id = m_idAllocator.allocateId();
        m_images.emplace(id, std::move(image));

        Logger::debug("Added image with id {}", id);
        return id;
    }

    void VulkanImageCache::disposeImage(ImageId id) {
        MOE_ASSERT(m_initialized, "VulkanImageCache not initialized");

        auto it = m_images.find(id);
        if (it != m_images.end()) {
            m_engine->destroyImage(it->second);
            m_images.erase(it);

            m_idAllocator.recycleId(id);

            Logger::debug("Disposed image with id {}", id);
        }
    }

    void VulkanImageCache::destroy() {
        MOE_ASSERT(m_initialized, "VulkanImageCache not initialized");

        for (auto& image: m_images) {
            m_engine->destroyImage(image.second);
        }

        m_images.clear();
        m_initialized = false;
    }
}// namespace moe