#include "Audio/AudioSource.hpp"

MOE_BEGIN_NAMESPACE

AudioSource::AudioSource() {
    alGenSources(1, &m_sourceId);
}

AudioSource::~AudioSource() {
    alDeleteSources(1, &m_sourceId);
}

void AudioSource::load(Ref<AudioDataProvider> provider, bool loop) {
    m_provider = provider;
    m_loop = loop;

    if (!m_provider->isStreaming()) {
        loadStaticBuffer();
    } else {
        loadStreamInitialBuffers();
    }
}

void AudioSource::play() {
    alSourcePlay(m_sourceId);
}

void AudioSource::pause() {
    alSourcePause(m_sourceId);
}

void AudioSource::stop() {
    alSourceStop(m_sourceId);
}

void AudioSource::update() {
    if (!m_provider->isStreaming()) {
        return;
    }

    unqueueProcessedBuffer();
    streamUpdate();

    ALint sourceState = 0;
    alGetSourcei(m_sourceId, AL_SOURCE_STATE, &sourceState);
    if (sourceState != AL_PLAYING) {
        alSourcePlay(m_sourceId);
    }
}

void AudioSource::loadStreamInitialBuffers() {
    for (size_t i = 0; i < MAX_STREAMING_BUFFERS; i++) {
        size_t packetSize = 0;
        Ref<AudioBuffer> buffer = m_provider->streamNextPacket(&packetSize);
        if (!buffer) {
            // eof reached
            break;
        }

        alSourceQueueBuffers(m_sourceId, 1, &buffer->bufferId);
        m_queuedBuffers.push(buffer);
    }
}

void AudioSource::loadStaticBuffer() {
    Ref<AudioBuffer> buffer = m_provider->loadStaticData();
    if (!buffer) {
        Logger::error("Failed to load static audio data");
        return;
    }

    alSourcei(m_sourceId, AL_BUFFER, static_cast<ALint>(buffer->bufferId));
}

void AudioSource::unqueueProcessedBuffer() {
    ALint processedCount = 0;
    alGetSourcei(m_sourceId, AL_BUFFERS_PROCESSED, &processedCount);

    while (processedCount-- > 0) {
        auto buf = m_queuedBuffers.front();
        alSourceUnqueueBuffers(m_sourceId, 1, &buf->bufferId);

        m_queuedBuffers.pop();
    }
}

void AudioSource::streamUpdate() {
    while (true) {
        ALint queuedCount = 0;
        alGetSourcei(m_sourceId, AL_BUFFERS_QUEUED, &queuedCount);
        if (queuedCount >= MAX_STREAMING_BUFFERS) {
            break;
        }

        size_t packetSize = 0;
        Ref<AudioBuffer> buffer = m_provider->streamNextPacket(&packetSize);
        if (!buffer) {
            // eof reached
            if (m_loop) {
                m_provider->seekToStart();
                buffer = m_provider->streamNextPacket(&packetSize);
                if (!buffer) {
                    break;
                }
            } else {
                break;
            }
        }

        alSourceQueueBuffers(m_sourceId, 1, &buffer->bufferId);
        m_queuedBuffers.push(buffer);
    }
}

MOE_END_NAMESPACE