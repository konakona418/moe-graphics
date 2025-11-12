#include "Audio/AudioBuffer.hpp"

MOE_BEGIN_NAMESPACE

void AudioBuffer::init() {
    MOE_ASSERT(bufferId == INVALID_BUFFER_ID, "AudioBuffer already initialized");
    alGenBuffers(1, &bufferId);
}

bool AudioBuffer::uploadData(Span<const uint8_t> data, ALenum format, ALsizei freq) {
    MOE_ASSERT(bufferId != INVALID_BUFFER_ID, "AudioBuffer not initialized");

    alGetError();
    alBufferData(
            bufferId,
            format,
            data.data(),
            static_cast<ALsizei>(data.size()),
            freq);

    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        Logger::error("Failed to upload audio buffer data: AL error code {}", error);
        return false;
    }
    return true;
}

void AudioBuffer::destroy() {
    MOE_ASSERT(bufferId != INVALID_BUFFER_ID, "AudioBuffer not initialized");
    alDeleteBuffers(1, &bufferId);
    bufferId = INVALID_BUFFER_ID;
}

void AudioBufferPool::init() {
    MOE_ASSERT(m_instance == nullptr, "AudioBufferPool already initialized");
    m_instance = this;

    for (size_t i = 0; i < POOL_INIT_SIZE; i++) {
        allocBuffer();
    }
}

void AudioBufferPool::destroy() {
    MOE_ASSERT(m_instance == this, "AudioBufferPool instance mismatch");

    for (auto& buffer: m_buffers) {
        buffer->destroy();
    }
    m_buffers.clear();
    m_freeBuffers.clear();

    m_instance = nullptr;
}

Ref<AudioBuffer> AudioBufferPool::acquireBuffer() {
    if (m_freeBuffers.empty()) {
        // no pending buffers, allocate a new one
        allocBuffer();
    }

    AudioBuffer* buffer = m_freeBuffers.back();
    m_freeBuffers.pop_back();

    return Ref<AudioBuffer>(buffer);
}

void AudioBufferPool::bufferDeleter(void* ptr) {
    // Rc reached zero, return to pool
    AudioBufferPool::get()->m_freeBuffers.push_back(static_cast<AudioBuffer*>(ptr));
}

void AudioBufferPool::allocBuffer() {
    if (m_buffers.size() >= 128) {
        Logger::warn("AudioBufferPool allocated too many buffers({})", m_buffers.size());
    }

    auto buffer = makePinned<AudioBuffer>();
    buffer->init();
    buffer->setDeleter(AudioBufferPool::bufferDeleter);

    m_buffers.push_back(buffer);
    m_freeBuffers.push_back(buffer.get());
}

MOE_END_NAMESPACE
