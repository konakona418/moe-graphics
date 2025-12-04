#pragma once

#include "Audio/AudioDataProvider.hpp"
#include "Audio/Common.hpp"

#include "Core/Resource/BinaryBuffer.hpp"

#include <vorbis/vorbisfile.h>


MOE_BEGIN_NAMESPACE

struct StreamedOggProvider : public AudioDataProvider {
public:
    StreamedOggProvider(Ref<BinaryBuffer> oggData, size_t chunkSize);
    ~StreamedOggProvider() override;

    ALuint getFormat() const override;
    ALsizei getSampleRate() const override;

    bool isStreaming() const override { return true; }

    Ref<AudioBuffer> loadStaticData() override {
        MOE_ASSERT(false, "Load entire data region is not designed for streamed audio data");
        return Ref<AudioBuffer>(nullptr);
    }

    // this function streams the next packet of audio data
    // ! if eof, returns nullptr and outSize is 0
    Ref<AudioBuffer> streamNextPacket(size_t* outSize) override;

    void seekToStart() override;

private:
    struct OggReadContext {
        size_t m_currentOffset{0};
        Ref<BinaryBuffer> m_buffer;
    };

    OggVorbis_File m_oggFile;
    Ref<BinaryBuffer> m_oggData;
    size_t m_chunkSize;

    Pinned<OggReadContext> m_readContext;

    static ov_callbacks generateCallbacks();
};

MOE_END_NAMESPACE