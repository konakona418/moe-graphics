#include "Render/Vulkan/VulkanMaterialCache.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"


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

        initDefaults();
    }

    Optional<VulkanCPUMaterial> VulkanMaterialCache::getMaterial(MaterialId id) {
        MOE_ASSERT(m_initialized, "VulkanMaterialCache not initialized");
        if (isMaterialIdValid(id)) {
            return m_materials[id];
        }
        return std::nullopt;
    }

    void VulkanMaterialCache::loadMaterial(VulkanCPUMaterial material, MaterialId matId) {
        auto* gpuMaterial =
                static_cast<VulkanGPUMaterial*>(m_materialBuffer.vmaAllocationInfo.pMappedData);

        // todo: this requires textures must be valid.
        // ! fixme: work on this after texture cache is implemented
        gpuMaterial[matId] =
                VulkanGPUMaterial{
                        .baseColor = material.baseColor,
                        .metallicRoughnessEmissive = glm::vec4(material.metallic, material.roughness, material.emissive, 0.0f),
                        .emissiveColor = material.emissiveColor,
                        .diffuseTexture = material.diffuseTexture,
                        .normalTexture = material.normalTexture,
                        .metallicRoughnessTexture = material.metallicRoughnessTexture,
                        .emissiveTexture = material.emissiveTexture,
                };

        Logger::debug("Loaded material with id {}", matId);
    }

    MaterialId VulkanMaterialCache::loadMaterial(VulkanCPUMaterial material) {
        MOE_ASSERT(m_initialized, "VulkanMaterialCache not initialized");

        MaterialId id = allocateMaterialId();
        MOE_ASSERT(id < MAX_STATIC_MATERIAL_COUNT, "MaterialId out of bounds");

        m_materials[id] = material;
        loadMaterial(material, id);

        Logger::debug("Loaded material with id {}", id);

        return id;
    }

    void VulkanMaterialCache::resetDynamicState() {
        m_temporaryMaterialCount = 0;
    }

    MaterialId VulkanMaterialCache::loadTemporaryMaterial(VulkanCPUMaterial material) {
        MOE_ASSERT(m_initialized, "VulkanMaterialCache not initialized");
        MOE_ASSERT(m_temporaryMaterialCount < MAX_TEMPORARY_MATERIAL_COUNT, "Max temporary material count reached");

        MaterialId id = TEMPORARY_MATERIAL_START_ID + m_temporaryMaterialCount++;
        m_materials[id] = material;
        loadMaterial(material, id);

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
        m_idAllocator.reset();
        m_materials.clear();

        m_initialized = false;
        m_engine = nullptr;
    }

    void VulkanMaterialCache::recycleMaterialId(MaterialId id) {
        m_idAllocator.recycleId(id);
        Logger::debug("Recycling material id {}", id);
    }

    MaterialId VulkanMaterialCache::allocateMaterialId() {
        return m_idAllocator.allocateId();
    }

    bool VulkanMaterialCache::isMaterialIdValid(MaterialId id) {
        return m_idAllocator.isIdValid(id);
    }

    void VulkanMaterialCache::initDefaults() {
        Logger::debug("Initializing default materials...");

        {
            MOE_ASSERT(m_defaults.whiteMaterial == NULL_MATERIAL_ID, "Default materials already initialized");
            MOE_ASSERT(m_defaults.blackMaterial == NULL_MATERIAL_ID, "Default materials already initialized");
            MOE_ASSERT(m_defaults.checkerboardMaterial == NULL_MATERIAL_ID, "Default materials already initialized");
        }

        {
            VulkanCPUMaterial material{};
            material.baseColor = glm::vec4(1.0f);
            material.diffuseTexture = m_engine->m_caches.imageCache.getDefaultImage(VulkanImageCache::DefaultResourceType::White);
            m_defaults.whiteMaterial = loadMaterial(material);
        }
        {
            VulkanCPUMaterial material{};
            material.baseColor = glm::vec4(1.0f);
            material.diffuseTexture = m_engine->m_caches.imageCache.getDefaultImage(VulkanImageCache::DefaultResourceType::Black);
            m_defaults.blackMaterial = loadMaterial(material);
        }
        {
            VulkanCPUMaterial material{};
            material.baseColor = glm::vec4(1.0f);
            material.diffuseTexture = m_engine->m_caches.imageCache.getDefaultImage(VulkanImageCache::DefaultResourceType::Checkerboard);
            m_defaults.checkerboardMaterial = loadMaterial(material);
        }
    }
}// namespace moe