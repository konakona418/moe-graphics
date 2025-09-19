#pragma once

#include "Render/Vulkan/VulkanCacheUtils.hpp"
#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanMaterial.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


// fwd decl
namespace moe {
    class VulkanEngine;
}

namespace moe {
    struct VulkanMaterialCache {
    public:
        enum class DefaultResourceType {
            White,
            Black,
            Checkerboard
        };

        VulkanMaterialCache() = default;
        ~VulkanMaterialCache() = default;

        void init(VulkanEngine& engine);

        Optional<VulkanCPUMaterial> getMaterial(MaterialId id);

        MaterialId loadMaterial(VulkanCPUMaterial material);

        void disposeMaterial(MaterialId id);

        void destroy();

        VkDeviceAddress getMaterialBufferAddress() {
            MOE_ASSERT(m_initialized, "VulkanMaterialCache not initialized");
            MOE_ASSERT(m_materialBuffer.address != 0, "Material buffer not allocated");
            return m_materialBuffer.address;
        }

        MaterialId getDefaultMaterial(DefaultResourceType type) const {
            MOE_ASSERT(m_initialized, "VulkanMaterialCache not initialized");
            switch (type) {
                case DefaultResourceType::White:
                    return m_defaults.whiteMaterial;
                case DefaultResourceType::Black:
                    return m_defaults.blackMaterial;
                case DefaultResourceType::Checkerboard:
                    return m_defaults.checkerboardMaterial;
            }
            MOE_ASSERT(false, "Invalid default material type");
            return NULL_MATERIAL_ID;//make linter happy
        }

        MaterialId getMaterialMissingNo() const {
            MOE_ASSERT(m_initialized, "VulkanMaterialCache not initialized");
            return m_defaults.checkerboardMaterial;
        }

        MaterialId getMaterialDefaultWhite() const {
            MOE_ASSERT(m_initialized, "VulkanMaterialCache not initialized");
            return m_defaults.whiteMaterial;
        }

    private:
        constexpr static uint32_t MAX_MATERIAL_COUNT = 1024;

        struct {
            MaterialId whiteMaterial{NULL_MATERIAL_ID};
            MaterialId blackMaterial{NULL_MATERIAL_ID};
            MaterialId checkerboardMaterial{NULL_MATERIAL_ID};
        } m_defaults;

        bool m_initialized{false};
        VulkanEngine* m_engine{nullptr};
        UnorderedMap<MaterialId, VulkanCPUMaterial> m_materials;

        VulkanCacheIdAllocator<MaterialId> m_idAllocator;

        VulkanAllocatedBuffer m_materialBuffer;

        void recycleMaterialId(MaterialId id);
        MaterialId allocateMaterialId();
        bool isMaterialIdValid(MaterialId id);

        void initDefaults();
    };
}// namespace moe