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
            auto proj = glm::perspective(glm::radians(fovDeg), aspectRatio, nearZ, farZ);
            proj[1][1] *= -1;// vulkan NDC
            return proj;
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

        Array<glm::vec3, 8> getFrustumCornersWorldSpace(float aspectRatio) const {
            Array<glm::vec3, 8> corners;

            glm::mat4 inv = glm::inverse(projectionMatrix(aspectRatio) * viewMatrix());

            int index = 0;
            for (int x = -1; x <= 1; x += 2) {
                for (int y = -1; y <= 1; y += 2) {
                    for (int z = -1; z <= 1; z += 2) {
                        glm::vec4 pt = inv * glm::vec4((float) x, (float) y, (float) z, 1.0f);
                        pt /= pt.w;
                        corners[index++] = glm::vec3(pt);
                    }
                }
            }

            return corners;
        }

        struct CSMCameraTransform {
            glm::mat4 view;
            glm::mat4 proj;
            glm::mat4 viewProj;
        };

#ifdef MOE_USE_LEGACY_CSM_CAMERA_IMPLEMENTATION
        // legacy implementation. performance is not great
        static CSMCameraTransform getCSMCamera(Array<glm::vec3, 8> corners, glm::vec3 lightDir, int textureSize) {
            glm::vec3 center(0.0f);
            for (auto& c: corners) {
                center += c;
            }
            center /= 8.0f;

            glm::vec3 lightDirNorm = glm::normalize(lightDir);
            glm::vec3 lightPos = center - lightDirNorm * 100.0f;

            glm::mat4 lightView = glm::lookAt(lightPos, center, glm::vec3(0.0f, 1.0f, 0.0f));

            glm::vec3 minBounds(FLT_MAX);
            glm::vec3 maxBounds(-FLT_MAX);
            for (auto& c: corners) {
                glm::vec4 ls = lightView * glm::vec4(c, 1.0f);
                minBounds = glm::min(minBounds, glm::vec3(ls));
                maxBounds = glm::max(maxBounds, glm::vec3(ls));
            }

            const auto boundExpand = [](float& boundMin, float& boundMax) {
                MOE_ASSERT(boundMax >= boundMin, "Invalid bounds");
                float extent = boundMax - boundMin;
                float expand = extent * 0.5f;
                boundMin -= expand;
                boundMax += expand;
            };

            // ! hacky way. or some shadow can be clipped
            boundExpand(minBounds.x, maxBounds.x);
            boundExpand(minBounds.y, maxBounds.y);

            // expand before aligning to texel size
            // seem to work a little better

            // adjust bounds to align with texel size
            float worldUnitsPerTexel = (maxBounds.x - minBounds.x) / float(textureSize);
            minBounds.x = floor(minBounds.x / worldUnitsPerTexel) * worldUnitsPerTexel;
            minBounds.y = floor(minBounds.y / worldUnitsPerTexel) * worldUnitsPerTexel;
            maxBounds.x = floor(maxBounds.x / worldUnitsPerTexel) * worldUnitsPerTexel;
            maxBounds.y = floor(maxBounds.y / worldUnitsPerTexel) * worldUnitsPerTexel;

            // it seems that with this the shadow quality is a little bit more stable
            // but not much
            maxBounds.z = floor(maxBounds.z / worldUnitsPerTexel) * worldUnitsPerTexel;
            minBounds.z = floor(minBounds.z / worldUnitsPerTexel) * worldUnitsPerTexel;

            // ! fixme: this falloff does work well in some scenarios,
            // ! but i don't think it's a robust solution
            constexpr float zBound = 10.0f;
            float cascadeRange = std::max(0.001f, std::abs(minBounds.z - maxBounds.z));
            float zBoundFalloff = zBound / (1.0f + cascadeRange);

            glm::mat4 lightProj = glm::ortho(
                    minBounds.x, maxBounds.x,
                    minBounds.y, maxBounds.y,
                    -maxBounds.z - zBoundFalloff,
                    -minBounds.z + zBoundFalloff);
            lightProj[1][1] *= -1;// vulkan NDC

            CSMCameraTransform transform{
                    .view = lightView,
                    .proj = lightProj,
                    .viewProj = lightProj * lightView,
            };

            return transform;
        }
#else
        static constexpr glm::vec3 DEFAULT_CSM_CAMERA_CLIP_SPACE_SCALE = {2.0f, 2.0f, 2.0f};

        // new implementation, with more stable shadow quality
        static CSMCameraTransform getCSMCamera(
                Array<glm::vec3, 8> corners,
                glm::vec3 lightDir,
                int textureSize,
                glm::vec3 clipSpaceScale = DEFAULT_CSM_CAMERA_CLIP_SPACE_SCALE) {
            glm::vec3 minBounds(FLT_MAX);
            glm::vec3 maxBounds(-FLT_MAX);

            for (auto& c: corners) {
                glm::vec4 ls = glm::vec4(c, 1.0f);
                minBounds = glm::min(minBounds, glm::vec3(ls));
                maxBounds = glm::max(maxBounds, glm::vec3(ls));
            }

            glm::vec3 lightDirNorm = glm::normalize(lightDir);

            if (lightDirNorm.x == 0.0f && lightDirNorm.z == 0.0f) {
                // ! fixme: this is too hacky
                // avoid gimbal lock
                lightDirNorm.x = 0.001f;
            }

            glm::mat4 view = glm::lookAt({}, lightDirNorm, glm::vec3(0.0f, 1.0f, 0.0f));

            float radius = glm::distance(minBounds, maxBounds) / 2.0f;
            radius = std::floor(radius);

            glm::vec3 center = (minBounds + maxBounds) * 0.5f;

            glm::vec3 frustumCenter = glm::vec3{view * glm::vec4(center, 1.0f)};
            float texelsPerUnit = textureSize / (radius * 2.0f);

            float offsetX = std::fmod(frustumCenter.x, texelsPerUnit);
            float offsetY = std::fmod(frustumCenter.y, texelsPerUnit);

            frustumCenter -= glm::vec3{offsetX, offsetY, 0.0f};

            glm::vec3 frustumCenterWorld = glm::vec3{glm::inverse(view) * glm::vec4(frustumCenter, 1.0f)};

            glm::mat4 lightView = glm::lookAt(frustumCenterWorld, frustumCenterWorld + lightDirNorm, glm::vec3(0.0f, 1.0f, 0.0f));

            // this can be tuned
            // a value too small will clip some shadow
            // a value too large will reduce shadow precision
            // Testing results:
            // - for smaller scenes, 1.0f ~ 2.0f works well
            // - for larger scenes(dust2 test scene), 5.0f works well
            // maybe we can make this value dynamic based on the cascade range
            float scaleX = clipSpaceScale.x;
            float scaleY = clipSpaceScale.y;
            float scaleZ = clipSpaceScale.z;
            glm::mat4 lightProj = glm::ortho(
                    -radius * scaleX, radius * scaleX,
                    -radius * scaleY, radius * scaleY,
                    -radius * scaleZ, radius * scaleZ);
            lightProj[1][1] *= -1;// vulkan NDC

            CSMCameraTransform transform{
                    .view = lightView,
                    .proj = lightProj,
                    .viewProj = lightProj * lightView,
            };

            return transform;
        }
#endif

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

        VulkanCamera() = default;

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