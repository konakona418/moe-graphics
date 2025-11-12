#pragma once

#include "Audio/Common.hpp"

#include "Core/Common.hpp"
#include "Math/Common.hpp"


MOE_BEGIN_NAMESPACE

struct AudioListener {
public:
    void init();
    void destroy();

    void setPosition(const glm::vec3& pos);
    void setVelocity(const glm::vec3& vel);
    void setOrientation(const glm::vec3& forward, const glm::vec3& up = Math::VK_GLOBAL_UP);
    void setGain(float gain);
};

MOE_END_NAMESPACE
