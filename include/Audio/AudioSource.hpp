#pragma once

#include "Audio/AudioBuffer.hpp"
#include "Audio/Common.hpp"

#include "Core/RefCounted.hpp"

MOE_BEGIN_NAMESPACE

struct AudioSource : public RefCounted<AudioSource> {
public:
    AudioSource() = default;
    virtual ~AudioSource() = default;

    void queueBuffer(Ref<AudioBuffer> buffer);

    ALuint sourceId() const {
        return m_sourceId;
    }

private:
    ALuint m_sourceId{0};
    Queue<Ref<AudioBuffer>> m_queuedBuffers;
};

MOE_END_NAMESPACE