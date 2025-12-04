#include "Audio/AudioEngine.hpp"

MOE_BEGIN_NAMESPACE

void AudioEngine::init() {
    std::once_flag initFlag;
    std::call_once(initFlag, [this]() {
        MOE_ASSERT(!m_initialized, "AudioEngine already initialized");

        auto device = alcOpenDevice(nullptr);// default

        if (device) {
            const ALCchar* deviceName = alcGetString(device, ALC_DEVICE_SPECIFIER);
            Logger::info("Opened OpenAL device: {}", deviceName);

            auto context = alcCreateContext(device, nullptr);
            alcMakeContextCurrent(context);

            bool eaxSupported = alcIsExtensionPresent(device, "EAX2.0");
            m_eaxSupported = eaxSupported;
            if (eaxSupported) {
                Logger::info("EAX 2.0 is supported");
            } else {
                Logger::info("EAX 2.0 is not supported");
            }
        }

        if (alGetError() != AL_NO_ERROR) {
            Logger::error("OpenAL error occurred, failing to initialize audio engine");
            m_initialized = false;
            return;
        }

        Logger::info("Audio engine initialized successfully.");

        Logger::info("Initializing default audio listener...");
        m_listener.init();

        m_initialized = true;
    });
}

void AudioEngine::cleanup() {
    MOE_ASSERT(m_initialized, "AudioEngine not initialized");

    Logger::info("Cleaning up audio engine...");

    m_listener.destroy();

    auto context = alcGetCurrentContext();
    auto device = alcGetContextsDevice(context);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);

    Logger::info("Audio engine cleaned up.");
}

MOE_END_NAMESPACE