#pragma once

#include "Audio/Common.hpp"
#include <future>

MOE_BEGIN_NAMESPACE

class AudioEngine;

struct AudioCommand {
    virtual ~AudioCommand() = default;
    virtual void execute(AudioEngine& engine) = 0;
    virtual bool isBlocking() const { return false; }
};

struct BlockingAudioCommand : public AudioCommand {
public:
    virtual ~BlockingAudioCommand() = default;
    BlockingAudioCommand(std::promise<void>&& promise)
        : m_promise(std::move(promise)) {};

    bool isBlocking() const override { return true; }

    std::promise<void> m_promise;

    void notify() {
        m_promise.set_value();
    }
};

MOE_END_NAMESPACE