#pragma once

#include "Core/ResourceCache.hpp"

#include "Render/Vulkan/VulkanSkeleton.hpp"

namespace moe {
    struct VulkanAnimationLoaderFunctor {
        SharedResource<VulkanSkeletonAnimation> operator()(VulkanSkeletonAnimation&& animation) {
            return std::make_shared<VulkanSkeletonAnimation>(std::forward<VulkanSkeletonAnimation>(animation));
        }
    };

    struct VulkanAnimationCache : public ResourceCache<AnimationId, VulkanSkeletonAnimation, VulkanAnimationLoaderFunctor> {};
}// namespace moe