#include "Render/Vulkan/VulkanMaterialCache.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"
#include "Render/Vulkan/VulkanLoaders.hpp"


namespace moe {
    void VulkanMaterialCache::init(VulkanEngine& engine) {
        m_engine = &engine;
        m_initialized = true;

        // ! fixme: hardcoded limit
        // ! fixme: memory usage
        m_materialBuffer = engine.allocateBuffer(
                MAX_MATERIAL_COUNT * sizeof(VulkanGPUMaterial),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_AUTO);

        // todo: load default textures
    }

    Optional<VulkanCPUMaterial> VulkanMaterialCache::getMaterial(MaterialId id) {
        MOE_ASSERT(m_initialized, "VulkanMaterialCache not initialized");
        if (isMaterialIdValid(id)) {
            return m_materials[id];
        }
        return std::nullopt;
    }

    MaterialId VulkanMaterialCache::loadMaterial(VulkanCPUMaterial material) {
        MOE_ASSERT(m_initialized, "VulkanMaterialCache not initialized");

        MaterialId id = allocateMaterialId();
        MOE_ASSERT(id < MAX_MATERIAL_COUNT, "MaterialId out of bounds");

        auto* gpuMaterial =
                static_cast<VulkanGPUMaterial*>(m_materialBuffer.vmaAllocationInfo.pMappedData);

        // todo: this requires textures must be valid.
        // ! fixme: work on this after texture cache is implemented
        gpuMaterial[id] =
                VulkanGPUMaterial{
                        .baseColor = material.baseColor,
                        .metallicRoughnessEmissive = glm::vec4(material.metallic, material.roughness, material.emmissive, 0.0f),
                        .diffuseTexture = material.diffuseTexture,
                        .normalTexture = material.normalTexture,
                        .metallicRoughnessTexture = material.metallicRoughnessTexture,
                        .emissiveTexture = material.emissiveTexture,
                };

        return id;
    }

    void VulkanMaterialCache::disposeMaterial(MaterialId id) {
        MOE_ASSERT(m_initialized, "VulkanMaterialCache not initialized");
        if (isMaterialIdValid(id)) {
            m_materials.erase(id);
            recycleMaterialId(id);
        } else {
            Logger::warn("Trying to dispose invalid material id {}", id);
        }
    }

    void VulkanMaterialCache::destroy() {
        m_engine->destroyBuffer(m_materialBuffer);
        m_recycledMaterialIds.clear();
        m_recycledMaterialIdLookup.clear();
        m_materials.clear();

        m_initialized = false;
        m_engine = nullptr;
    }

    void VulkanMaterialCache::recycleMaterialId(MaterialId id) {
        m_recycledMaterialIds.push_back(id);
        m_recycledMaterialIdLookup.insert(id);

        Logger::debug("Recycling material id {}", id);
    }

    MaterialId VulkanMaterialCache::allocateMaterialId() {
        if (!m_recycledMaterialIds.empty()) {
            MaterialId id = m_recycledMaterialIds.front();

            m_recycledMaterialIds.pop_front();
            m_recycledMaterialIdLookup.erase(id);

            Logger::info("Reusing recycled material id {}", id);
            return id;
        }
        return m_nextMaterialId++;
    }

    bool VulkanMaterialCache::isMaterialIdValid(MaterialId id) {
        return m_recycledMaterialIdLookup.find(id) == m_recycledMaterialIdLookup.end() &&
               id < m_nextMaterialId;
    }
}// namespace moe