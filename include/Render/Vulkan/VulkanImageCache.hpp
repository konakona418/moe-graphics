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
        VulkanImageCache() = default;
        ~VulkanImageCache() = default;

        void init(VulkanEngine& engine);

        ImageId addImage(VulkanAllocatedImage&& image);

        void disposeImage(ImageId id);

        void destroy();

    private:
        bool m_initialized{false};
        VulkanEngine* m_engine{nullptr};

        UnorderedMap<ImageId, VulkanAllocatedImage> m_images;
        VulkanCacheIdAllocator<ImageId> m_idAllocator;
    };
}// namespace moe