#pragma once

#include "Audio/AudioBuffer.hpp"
#include "Audio/Common.hpp"

#include "Core/RefCounted.hpp"


MOE_BEGIN_NAMESPACE

struct AudioDataProvider : public RefCounted<AudioDataProvider> {
    virtual ~AudioDataProvider() = default;

    virtual ALuint getFormat() const = 0;
    virtual ALsizei getSampleRate() const = 0;

    virtual bool isStreaming() const = 0;

    virtual Ref<AudioBuffer> loadStaticData() = 0;
    virtual Ref<AudioBuffer> streamNextPacket(size_t* outSize) = 0;

    virtual void seekToStart() = 0;
};

MOE_END_NAMESPACE