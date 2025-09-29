#pragma once

#include "Math/Common.hpp"

namespace moe {
    struct Transform {
    public:
        Transform()
            : position(0.0f, 0.0f, 0.0f), rotation(1.0f), scale(1.0f, 1.0f, 1.0f) {}

        Transform& setPosition(glm::vec3 pos) {
            position = pos;
            return *this;
        }

        Transform& setRotation(glm::mat4 rot) {
            rotation = rot;
            return *this;
        }

        Transform& setRotation(glm::quat rot) {
            rotation = glm::mat4_cast(rot);
            return *this;
        }

        Transform& setRotation(glm::vec3 axis, float angleRad) {
            rotation = glm::rotate(glm::mat4(1.0f), angleRad, axis);
            return *this;
        }

        Transform& setScale(glm::vec3 scale) {
            this->scale = scale;
            return *this;
        }

        glm::mat4 getMatrix() const {
            // T * R * S
            return glm::translate(glm::mat4(1.0f), position) * rotation * glm::scale(glm::mat4(1.0f), scale);
        }

    private:
        glm::vec3 position;
        glm::mat4 rotation;
        glm::vec3 scale;
    };
}// namespace moe