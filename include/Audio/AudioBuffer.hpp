#pragma once

#include "Audio/Common.hpp"

#include "Core/Common.hpp"
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

struct AudioBufferPool {
public:
    static constexpr size_t POOL_INIT_SIZE = 32;

    AudioBufferPool() = default;
    ~AudioBufferPool() = default;

    void init();

    void destroy();

    Ref<AudioBuffer> acquireBuffer();

    static AudioBufferPool* get() {
        MOE_ASSERT(m_instance != nullptr, "AudioBufferPool not initialized");
        return m_instance;
    }

private:
    Vector<Pinned<AudioBuffer>> m_buffers;
    Deque<AudioBuffer*> m_freeBuffers;

    static AudioBufferPool* m_instance;
    static void bufferDeleter(void* ptr);

    void allocBuffer();
};

MOE_END_NAMESPACE
