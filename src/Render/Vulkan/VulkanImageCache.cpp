#include "Render/Vulkan/VulkanImageCache.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"


namespace moe {
    void VulkanImageCache::init(VulkanEngine& engine) {
        m_engine = &engine;
        m_initialized = true;

        initDefaults();
    }

    ImageId VulkanImageCache::addImage(VulkanAllocatedImage&& image) {
        MOE_ASSERT(m_initialized, "VulkanImageCache not initialized");

        ImageId id = m_idAllocator.allocateId();
        m_images.emplace(id, std::move(image));

        m_engine->getBindlessSet().addImage(id, m_images[id].imageView);

        Logger::debug("Added image with id {}", id);
        return id;
    }

    ImageId VulkanImageCache::loadImageFromFile(StringView filename, VkFormat format, VkImageUsageFlags usage, bool mipmap) {
        MOE_ASSERT(m_initialized, "VulkanImageCache not initialized");

        auto image = m_engine->loadImageFromFile(filename, format, usage, mipmap);
        if (!image) {
            Logger::error("Failed to load image from file: {}", filename);
            return NULL_IMAGE_ID;
        }

        return addImage(std::move(image.value()));
    }

    void VulkanImageCache::disposeImage(ImageId id) {
        MOE_ASSERT(m_initialized, "VulkanImageCache not initialized");

        auto it = m_images.find(id);
        if (it != m_images.end()) {
            m_engine->destroyImage(it->second);
            m_images.erase(it);

            // ! fixme: bindless set did not implement image removal
            // ! it's possible to implement an update image method here

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

    void VulkanImageCache::initDefaults() {
        Logger::debug("Initializing default images...");

        {
            MOE_ASSERT(m_defaults.whiteImage == NULL_IMAGE_ID, "Default images already initialized");
            MOE_ASSERT(m_defaults.blackImage == NULL_IMAGE_ID, "Default images already initialized");
            MOE_ASSERT(m_defaults.checkerboardImage == NULL_IMAGE_ID, "Default images already initialized");
        }

        {
            uint32_t color = glm::packUnorm4x8(glm::vec4(1.0f));
            auto image = m_engine->allocateImage(
                    &color,
                    {1, 1, 1},
                    VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_SAMPLED_BIT);

            m_defaults.whiteImage = addImage(std::move(image));
        }
        {
            uint32_t color = glm::packUnorm4x8(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            auto image = m_engine->allocateImage(
                    &color,
                    {1, 1, 1},
                    VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_SAMPLED_BIT);

            m_defaults.blackImage = addImage(std::move(image));
        }
        {
            uint32_t magenta = glm::packUnorm4x8(glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
            uint32_t black = glm::packUnorm4x8(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

            Array<uint32_t, 16 * 16> checkerboard{};
            for (int x = 0; x < 16; ++x) {
                for (int y = 0; y < 16; ++y) {
                    checkerboard[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
                }
            }

            auto image = m_engine->allocateImage(
                    checkerboard.data(),
                    {16, 16, 1},
                    VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_SAMPLED_BIT);

            m_defaults.checkerboardImage = addImage(std::move(image));
        }
        {
            uint32_t flatNormal = glm::packUnorm4x8(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));
            auto image = m_engine->allocateImage(
                    &flatNormal,
                    {1, 1, 1},
                    VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_SAMPLED_BIT);

            m_defaults.flatNormalImage = addImage(std::move(image));
        }
    }
}// namespace moe