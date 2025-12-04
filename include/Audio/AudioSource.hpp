#pragma once

#include "Audio/AudioBuffer.hpp"
#include "Audio/AudioDataProvider.hpp"
#include "Audio/Common.hpp"

#include "Core/RefCounted.hpp"

MOE_BEGIN_NAMESPACE

struct AudioSource : public RefCounted<AudioSource> {
public:
    static constexpr size_t MAX_STREAMING_BUFFERS = 4;

    AudioSource();
    ~AudioSource();

    void load(Ref<AudioDataProvider> provider, bool loop = false);

    void play();
    void pause();
    void stop();

    void update();

    ALuint sourceId() const {
        return m_sourceId;
    }

private:
    ALuint m_sourceId{0};

    Ref<AudioDataProvider> m_provider;
    bool m_loop{false};

    Queue<Ref<AudioBuffer>> m_queuedBuffers;

    void loadStreamInitialBuffers();
    void loadStaticBuffer();

    void streamUpdate();
    void unqueueProcessedBuffer();
};

MOE_END_NAMESPACE