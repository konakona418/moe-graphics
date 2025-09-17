#pragma once

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
        VulkanMaterialCache() = default;
        ~VulkanMaterialCache() = default;

        void init(VulkanEngine& engine);

        Optional<VulkanCPUMaterial> getMaterial(MaterialId id);

        MaterialId loadMaterial(VulkanCPUMaterial material);

        void disposeMaterial(MaterialId id);

        void destroy();

    private:
        constexpr static uint32_t MAX_MATERIAL_COUNT = 1024;

        bool m_initialized{false};
        VulkanEngine* m_engine{nullptr};
        MaterialId m_nextMaterialId{0};
        UnorderedMap<MaterialId, VulkanCPUMaterial> m_materials;

        Deque<MaterialId> m_recycledMaterialIds;
        UnorderedSet<MaterialId> m_recycledMaterialIdLookup;

        VulkanAllocatedBuffer m_materialBuffer;

        void recycleMaterialId(MaterialId id);
        MaterialId allocateMaterialId();
        bool isMaterialIdValid(MaterialId id);
    };
}// namespace moe