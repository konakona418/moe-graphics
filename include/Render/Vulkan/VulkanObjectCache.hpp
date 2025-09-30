#pragma once

#include "Core/ResourceCache.hpp"
#include "Render/Common.hpp"
#include "Render/Vulkan/VulkanLoaders.hpp"
#include "Render/Vulkan/VulkanRenderable.hpp"
#include "Render/Vulkan/VulkanScene.hpp"


namespace moe {

    struct ObjectLoader {
        struct GltfT {};
        static constexpr GltfT Gltf{};

        SharedResource<VulkanRenderable> operator()(VulkanEngine* engine, StringView filename, GltfT) {
            return std::make_shared<VulkanScene>(VkLoaders::GLTF::loadSceneFromFile(*engine, filename).value());
        };
    };

    struct VulkanObjectCache : ResourceCache<RenderableId, VulkanRenderable, ObjectLoader> {};

}// namespace moe