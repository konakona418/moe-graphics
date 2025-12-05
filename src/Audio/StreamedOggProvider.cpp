#include "Audio/StreamedOggProvider.hpp"
#include "Audio/AudioEngine.hpp"

#include "Math/Util.hpp"

MOE_BEGIN_NAMESPACE

ov_callbacks StreamedOggProvider::generateCallbacks() {
    ov_callbacks callbacks;

    callbacks.read_func = [](void* ptr, size_t size, size_t nmemb, void* datasource) -> size_t {
        StreamedOggProvider::OggReadContext* context =
                reinterpret_cast<StreamedOggProvider::OggReadContext*>(datasource);
        size_t bytesToRead = size * nmemb;
        size_t availableBytes =
                context->m_buffer->size() - context->m_currentOffset;
        size_t bytesActuallyRead = Math::min(bytesToRead, availableBytes);

        if (bytesActuallyRead > 0) {
            memcpy(
                    ptr,
                    context->m_buffer->data() + context->m_currentOffset,
                    bytesActuallyRead);
            context->m_currentOffset += bytesActuallyRead;
        }
        return bytesActuallyRead / size;
    };

    callbacks.seek_func = [](void* datasource, ogg_int64_t offset, int whence) -> int {
        StreamedOggProvider::OggReadContext* context =
                reinterpret_cast<StreamedOggProvider::OggReadContext*>(datasource);
        size_t newOffset = 0;

        switch (whence) {
            case SEEK_SET:
                newOffset = static_cast<size_t>(offset);
                break;
            case SEEK_CUR:
                newOffset = context->m_currentOffset + static_cast<size_t>(offset);
                break;
            case SEEK_END:
                newOffset = context->m_buffer->size() + static_cast<size_t>(offset);
                break;
            default:
                return -1;
        }

        if (newOffset > context->m_buffer->size()) {
            return -1;
        }

        context->m_currentOffset = newOffset;
        return 0;
    };

    callbacks.close_func = [](void* /*datasource*/) -> int {
        // nop
        return 0;
    };

    callbacks.tell_func = [](void* datasource) -> long {
        StreamedOggProvider::OggReadContext* context =
                reinterpret_cast<StreamedOggProvider::OggReadContext*>(datasource);
        return static_cast<long>(context->m_currentOffset);
    };

    return callbacks;
}

StreamedOggProvider::StreamedOggProvider(Ref<BinaryBuffer> oggData, size_t chunkSize)
    : m_oggData(oggData), m_chunkSize(chunkSize) {
    memset(&m_oggFile, 0, sizeof(m_oggFile));

    OggVorbis_File& oggFile = m_oggFile;
    ov_callbacks callbacks = generateCallbacks();

    m_readContext = makePinned<OggReadContext>(OggReadContext{
            0,
            oggData,
    });

    if (ov_open_callbacks(m_readContext.get(), &oggFile, nullptr, 0, callbacks) < 0) {
        Logger::error("Failed to open Ogg Vorbis data");
    }
}

StreamedOggProvider::~StreamedOggProvider() {
    ov_clear(&m_oggFile);
}

ALuint StreamedOggProvider::getFormat() const {
    vorbis_info* info = ov_info(const_cast<OggVorbis_File*>(&m_oggFile), -1);
    if (info->channels == 1) {
        return AL_FORMAT_MONO16;
    } else if (info->channels == 2) {
        return AL_FORMAT_STEREO16;
    } else {
        Logger::error("Unsupported number of channels in Ogg Vorbis data: {}", info->channels);
        return AL_FORMAT_MONO16;
    }
}

ALsizei StreamedOggProvider::getSampleRate() const {
    vorbis_info* info = ov_info(const_cast<OggVorbis_File*>(&m_oggFile), -1);
    return static_cast<ALsizei>(info->rate);
}

Ref<AudioBuffer> StreamedOggProvider::streamNextPacket(size_t* outSize) {
    auto& buffer = m_tempReadBuffer;
    buffer.resize(m_chunkSize);
    const size_t bufferSize = m_chunkSize;

    size_t totalBytesRead = 0;
    while (totalBytesRead < bufferSize) {
        long bytesRead = ov_read(
                const_cast<OggVorbis_File*>(&m_oggFile),
                reinterpret_cast<char*>(buffer.data() + totalBytesRead),
                static_cast<int>(bufferSize - totalBytesRead),
                0,// little endian
                2,// 16 bits per sample
                1,// signed data
                nullptr);
        if (bytesRead < 0) {
            Logger::error("Error while reading Ogg Vorbis data");
            return Ref<AudioBuffer>(nullptr);
        } else if (bytesRead == 0) {
            // eof
            break;
        }
        totalBytesRead += static_cast<size_t>(bytesRead);
    }

    if (totalBytesRead == 0) {
        // eof, return null
        return Ref<AudioBuffer>(nullptr);
    }

    // Logger::debug("Streamed {} bytes of Ogg Vorbis data", bytesRead);

    // alloc audio buffer and upload data
    Ref<AudioBuffer> audioBuffer =
            AudioEngine::getInstance()
                    .getBufferPool()
                    .acquireBuffer();

    if (!audioBuffer->uploadData(
                Span<const uint8_t>(
                        buffer.data(),
                        static_cast<size_t>(totalBytesRead)),
                getFormat(), getSampleRate())) {
        Logger::error("Failed to upload streamed Ogg Vorbis data to AudioBuffer");
        return Ref<AudioBuffer>(nullptr);
    }

    if (outSize) {
        *outSize = static_cast<size_t>(totalBytesRead);
    }
    return audioBuffer;
}

void StreamedOggProvider::seekToStart() {
    if (ov_pcm_seek(const_cast<OggVorbis_File*>(&m_oggFile), 0) != 0) {
        Logger::error("Failed to seek to start of Ogg Vorbis data");
    }
}

MOE_END_NAMESPACE