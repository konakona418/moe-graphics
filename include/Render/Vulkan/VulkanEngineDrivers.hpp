#pragma once

#include "Render/Common.hpp"
#include "Render/Vulkan/VulkanLight.hpp"
#include "Render/Vulkan/VulkanSwapBuffer.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"


namespace moe {
    class VulkanEngine;

    struct VulkanIlluminationBus {
    public:
        enum class RegistryType {
            Static,
            Dynamic
        };

        void init(VulkanEngine& engine);

        void destroy();

        VulkanIlluminationBus& addLight(const VulkanCPULight& light, RegistryType type);

        VulkanIlluminationBus& addStaticLight(const VulkanCPULight& light) {
            return addLight(light, RegistryType::Static);
        }

        VulkanIlluminationBus& addDynamicLight(const VulkanCPULight& light) {
            return addLight(light, RegistryType::Dynamic);
        }

        VulkanIlluminationBus& setAmbient(const glm::vec3& color, float strength) {
            ambientColorStrength = glm::vec4(color, strength);
            return *this;
        }

        VulkanIlluminationBus& setSunlight(const glm::vec3& direction, const glm::vec3& color, float strength);

        VulkanSwapBuffer& getGPUBuffer() { return lightBuffer; }

        VulkanCPULight getSunlight() const { return sunLight; }

        glm::vec4 getAmbientColorStrength() const { return ambientColorStrength; }

        void uploadToGPU(VkCommandBuffer cmdBuffer, uint32_t frameIndex);

        // includes the sun light + static lights + dynamic lights
        size_t getNumLights() const { return staticLights.size() + dynamicLights.size() + 1; }

        void resetDynamicState() { dynamicLights.clear(); }

        void resetAllState() {
            staticLights.clear();
            dynamicLights.clear();
        }

    private:
        VulkanEngine* m_engine{nullptr};
        glm::vec4 ambientColorStrength{1.0f, 1.0f, 1.0f, 1.0f};// rgb color, a strength
        VulkanCPULight sunLight;
        glm::vec3 sunLightDirection;

        Vector<VulkanCPULight> staticLights;
        Vector<VulkanCPULight> dynamicLights;

        VulkanSwapBuffer lightBuffer;
    };

    struct VulkanRenderObjectBus {
    public:
        VulkanRenderObjectBus() = default;
        ~VulkanRenderObjectBus() = default;

        void init(VulkanEngine& engine) {
            m_engine = &engine;
            m_initialized = true;
        }

        void destroy() {
            m_engine = nullptr;
            m_initialized = false;
        }

        VulkanRenderObjectBus& submitRender(RenderableId id, Transform transform) {
            MOE_ASSERT(m_initialized, "VulkanRenderObjectBus not initialized");
            if (m_renderCommands.size() >= MAX_RENDER_COMMANDS) {
                Logger::warn("Render object bus reached max render commands(4096), check if dynamic state is reset properly; exceeding commands will be ignored");
                return *this;
            }
            m_renderCommands.push_back({id, transform});
            return *this;
        }

        VulkanRenderObjectBus& submitRender(RenderCommand command) {
            MOE_ASSERT(m_initialized, "VulkanRenderObjectBus not initialized");
            if (m_renderCommands.size() >= MAX_RENDER_COMMANDS) {
                Logger::warn("Render object bus reached max render commands(4096), check if dynamic state is reset properly; exceeding commands will be ignored");
                return *this;
            }
            m_renderCommands.push_back(command);
            return *this;
        }

        void resetDynamicState() { m_renderCommands.clear(); }

        Deque<RenderCommand>& getRenderCommands() { return m_renderCommands; }

    private:
        static constexpr uint32_t MAX_RENDER_COMMANDS = 4096;

        VulkanEngine* m_engine{nullptr};
        bool m_initialized{false};

        Deque<RenderCommand> m_renderCommands;
    };
}// namespace moe
