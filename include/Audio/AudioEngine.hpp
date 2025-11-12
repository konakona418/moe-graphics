#pragma once

#include "Audio/Common.hpp"

#include "Audio/AudioListener.hpp"

MOE_BEGIN_NAMESPACE

class AudioEngine {
public:
    void init();
    void cleanup();

    static AudioEngine* get() {
        MOE_ASSERT(m_instance != nullptr, "AudioEngine not initialized");
        return m_instance;
    }

    AudioListener& getListener() {
        return m_listener;
    }

private:
    bool m_initialized{false};
    bool m_eaxSupported{false};

    static AudioEngine* m_instance;

    AudioListener m_listener;
};

MOE_END_NAMESPACE