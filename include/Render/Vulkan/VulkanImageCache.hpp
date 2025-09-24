#pragma once

#include "Render/Vulkan/VulkanCacheUtils.hpp"
#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

// fwd decl
namespace moe {
    class VulkanEngine;
}

namespace moe {
    struct VulkanImageCache {
    public:
        enum class DefaultResourceType {
            White,
            Black,
            Checkerboard,
            FlatNormal,
        };

        VulkanImageCache() = default;
        ~VulkanImageCache() = default;

        void init(VulkanEngine& engine);

        Optional<VulkanAllocatedImage> getImage(ImageId id);

        ImageId addImage(VulkanAllocatedImage&& image);

        ImageId loadImageFromFile(StringView filename, VkFormat format, VkImageUsageFlags usage, bool mipmap = false);

        ImageId loadCubeMapFromFiles(Array<StringView, 6> filenames, VkFormat format, VkImageUsageFlags usage, bool mipmap = false);

        void disposeImage(ImageId id);

        void destroy();

        ImageId getDefaultImage(DefaultResourceType type) const {
            MOE_ASSERT(m_initialized, "VulkanImageCache not initialized");
            switch (type) {
                case DefaultResourceType::White:
                    return m_defaults.whiteImage;
                case DefaultResourceType::Black:
                    return m_defaults.blackImage;
                case DefaultResourceType::Checkerboard:
                    return m_defaults.checkerboardImage;
                case DefaultResourceType::FlatNormal:
                    return m_defaults.flatNormalImage;
            }
            MOE_ASSERT(false, "Invalid default image type");
            return NULL_IMAGE_ID;//make linter happy
        }

    private:
        bool m_initialized{false};
        VulkanEngine* m_engine{nullptr};

        struct {
            ImageId whiteImage{NULL_IMAGE_ID};
            ImageId blackImage{NULL_IMAGE_ID};
            ImageId checkerboardImage{NULL_IMAGE_ID};
            ImageId flatNormalImage{NULL_IMAGE_ID};
        } m_defaults;

        UnorderedMap<ImageId, VulkanAllocatedImage> m_images;
        VulkanCacheIdAllocator<ImageId> m_idAllocator;

        void initDefaults();
    };
}// namespace moe