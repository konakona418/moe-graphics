#pragma once

#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    struct VulkanCamera {
        static constexpr float PITCH_LIMIT = 89.0f;
        VulkanCamera(glm::vec3 pos, float pitch = 0.0f, float yaw = 0.0f, float fovDeg = 45.0f, float nearZ = 0.1f, float farZ = 100.0f)
            : pos(pos), pitch(pitch), yaw(yaw), fovDeg(fovDeg), nearZ(nearZ), farZ(farZ) {
            updateVectors();
        }

        glm::mat4 viewMatrix() const {
            return glm::lookAt(pos, pos + front, up);
        }
        glm::mat4 projectionMatrix(float aspectRatio) const {
            return glm::perspective(glm::radians(fovDeg), aspectRatio, nearZ, farZ);
        }

        void setPosition(const glm::vec3& newPos) { pos = newPos; }
        const glm::vec3& getPosition() const { return pos; }

        const glm::vec3& getFront() const { return front; }

        const glm::vec3& getUp() const { return up; }

        const glm::vec3& getRight() const { return right; }

        const float getFovDeg() const { return fovDeg; }

        const float getNearZ() const { return nearZ; }

        const float getFarZ() const { return farZ; }

        void setPitch(float newPitch) {
            pitch = glm::clamp(newPitch, -PITCH_LIMIT, PITCH_LIMIT);
            updateVectors();
        }
        float getPitch() const { return pitch; }

        float getYaw() const { return yaw; }
        void setYaw(float newYaw) {
            yaw = newYaw;
            updateVectors();
        }

    private:
        glm::vec3 pos{0.0f};
        glm::vec3 front{0.0f, 0.0f, -1.0f};
        glm::vec3 up{0.0f, 1.0f, 0.0f};
        glm::vec3 right{1.0f, 0.0f, 0.0f};

        float pitch{0.0f};
        float yaw{0.0f};

        float fovDeg{45.0f};
        float nearZ{0.1f};
        float farZ{100.0f};

        void updateVectors() {
            glm::vec3 newFront;
            newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            newFront.y = sin(glm::radians(pitch));
            newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

            front = glm::normalize(newFront);
            right = glm::normalize(glm::cross(front, up));
        }
    };
}// namespace moe