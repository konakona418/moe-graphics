#include "Audio/AudioSource.hpp"
#include "Audio/AudioEngine.hpp"
#include <algorithm>

MOE_BEGIN_NAMESPACE

AudioSource::AudioSource() {
    alGenSources(1, &m_sourceId);
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
    if (!m_provider) {
        // n/a
        return;
    }

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
    if (!m_provider) {
        // n/a
        return;
    }

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

static void sourceDeleter(void* ptr) {
    AudioEngine::getInterface()
            .submitCommand(
                    std::make_unique<DestroySourceCommand>(
                            static_cast<AudioSource*>(ptr)));
}

void CreateSourceCommand::execute(AudioEngine& engine) {
    Pinned<AudioSource> sourcePinned = makePinned<AudioSource>();

    auto source = Ref<AudioSource>(sourcePinned.get());
    source->setDeleter(sourceDeleter);

    engine.getSources().push_back(std::move(sourcePinned));

    *outSource = source;
}

void DestroySourceCommand::execute(AudioEngine& engine) {
    auto id = source->sourceId();

    source->manualDestroy();

    auto& sources = engine.getSources();
    sources.erase(
            std::remove_if(
                    sources.begin(),
                    sources.end(),
                    [this](const Pinned<AudioSource>& s) {
                        return s.get() == source;
                    }),
            sources.end());

    Logger::debug("Audio source id {} removed", id);
}

void SourceLoadCommand::execute(AudioEngine& engine) {
    Logger::debug("Loading audio data for source {}", source->sourceId());
    source->load(provider, loop);
}

void SourcePlayCommand::execute(AudioEngine& engine) {
    Logger::debug("Play audio source {}", source->sourceId());
    source->play();
}

void SourcePauseCommand::execute(AudioEngine& engine) {
    Logger::debug("Pause audio source {}", source->sourceId());
    source->pause();
}

void SourceStopCommand::execute(AudioEngine& engine) {
    Logger::debug("Stop audio source {}", source->sourceId());
    source->stop();
}

MOE_END_NAMESPACE