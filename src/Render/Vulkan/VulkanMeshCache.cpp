#include "Render/Vulkan/VulkanMeshCache.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"
#include "Render/Vulkan/VulkanLoaders.hpp"


namespace moe {
    void VulkanMeshCache::init(VulkanEngine& engine) {
        m_engine = &engine;
        m_initialized = true;
    }

    MeshId VulkanMeshCache::loadMesh(VulkanMeshAsset mesh) {
        MOE_ASSERT(m_initialized, "VulkanMeshCache not initialized");

        MeshId id = m_idAllocator.allocateId();
        m_meshes.emplace(id, mesh);

        Logger::debug("Loaded mesh with id {}", id);
        return id;
    }

    MeshId VulkanMeshCache::loadMeshFromFile(StringView filename) {
        MOE_ASSERT(m_initialized, "VulkanMeshCache not initialized");

        auto mesh =
                VkLoaders::loadGLTFMeshFromFile(*m_engine, filename);
        if (!mesh.has_value()) {
            Logger::error("Failed to load mesh from file {}", filename);
            return NULL_MESH_ID;
        }

        return loadMesh(mesh.value());
    }

    Optional<VulkanMeshAsset> VulkanMeshCache::getMesh(MeshId id) const {
        MOE_ASSERT(m_initialized, "VulkanMeshCache not initialized");

        auto it = m_meshes.find(id);
        if (it != m_meshes.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    void VulkanMeshCache::destroy() {
        for (auto& mesh: m_meshes) {
            for (auto& submesh: mesh.second.meshes) {
                m_engine->destroyBuffer(submesh->gpuBuffer.vertexBuffer);
                m_engine->destroyBuffer(submesh->gpuBuffer.indexBuffer);
            }
        }

        m_idAllocator.reset();
        m_meshes.clear();

        m_engine = nullptr;
        m_initialized = false;
    }
}// namespace moe