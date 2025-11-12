#pragma once

#include "Audio/Common.hpp"

MOE_BEGIN_NAMESPACE

class AudioEngine {
public:
    void init();
    void cleanup();

    static AudioEngine* get() {
        MOE_ASSERT(m_instance != nullptr, "AudioEngine not initialized");
        return m_instance;
    }

private:
    bool m_initialized{false};
    bool m_eaxSupported{false};

    static AudioEngine* m_instance;
};

MOE_END_NAMESPACE