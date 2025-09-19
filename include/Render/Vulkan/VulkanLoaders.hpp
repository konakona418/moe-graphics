#pragma once

#include "Render/Vulkan/VulkanMesh.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

#include <stb_image.h>

namespace moe {
    class VulkanEngine;
    struct VulkanScene;

    namespace VkLoaders {
        Optional<VulkanMeshAsset> loadGLTFMeshFromFile(VulkanEngine& engine, StringView filename);

        struct StbiImageDeleter {
            void operator()(uint8_t* ptr) const {
                stbi_image_free(ptr);
            }
        };

        using UniqueRawImage = std::unique_ptr<uint8_t, StbiImageDeleter>;

        UniqueRawImage loadImage(StringView filename, int* width, int* height, int* channels);

        namespace GLTF {
            Optional<VulkanScene> loadSceneFromFile(VulkanEngine& engine, StringView filename);
        }
    }// namespace VkLoaders
}// namespace moe