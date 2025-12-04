#pragma once

#include "Audio/AudioListener.hpp"
#include "Audio/Common.hpp"


#include "Core/Meta/Feature.hpp"

MOE_BEGIN_NAMESPACE

class AudioEngine : public Meta::Singleton<AudioEngine> {
public:
    MOE_SINGLETON(AudioEngine)

    AudioListener& getListener() {
        return m_listener;
    }

private:
    bool m_initialized{false};
    bool m_eaxSupported{false};

    AudioListener m_listener;

    AudioEngine() {
        init();
    };

    ~AudioEngine() {
        cleanup();
    }

    void init();
    void cleanup();
};

MOE_END_NAMESPACE