#pragma once

#include "Audio/Common.hpp"

#include "Core/Common.hpp"
#include "Core/Meta/Feature.hpp"
#include "Core/RefCounted.hpp"


MOE_BEGIN_NAMESPACE

constexpr ALuint INVALID_BUFFER_ID = std::numeric_limits<ALuint>::max();

struct AudioBuffer : public RefCounted<AudioBuffer> {
public:
    ALuint bufferId{INVALID_BUFFER_ID};

    void init();
    bool uploadData(Span<const uint8_t> data, ALenum format, ALsizei freq);
    void destroy();
};

struct AudioBufferPool : public Meta::Singleton<AudioBufferPool> {
public:
    MOE_SINGLETON(AudioBufferPool)

    static constexpr size_t POOL_INIT_SIZE = 32;

    void init();

    void destroy();

    Ref<AudioBuffer> acquireBuffer();

private:
    Vector<Pinned<AudioBuffer>> m_buffers;
    Deque<AudioBuffer*> m_freeBuffers;
    bool m_initialized{false};

    static void bufferDeleter(void* ptr);

    AudioBufferPool() = default;
    ~AudioBufferPool() = default;

    void allocBuffer();
};

MOE_END_NAMESPACE
