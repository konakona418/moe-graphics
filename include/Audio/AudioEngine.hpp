#pragma once

#include "Audio/Common.hpp"

MOE_BEGIN_NAMESPACE

class AudioEngine {
public:
    void init();
    void cleanup();

private:
    bool m_initialized{false};
    bool m_eaxSupported{false};
};

MOE_END_NAMESPACE