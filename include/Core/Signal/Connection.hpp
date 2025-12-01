#pragma once

#include "Core/Common.hpp"
#include "Core/Meta/Feature.hpp"

#include "Core/Ref.hpp"
#include "Core/Signal/Common.hpp"

MOE_BEGIN_NAMESPACE

template<typename>
struct Signal;

template<typename SlotT>
struct Connection : public Meta::NonCopyable<Connection<SlotT>> {
public:
    Connection(Signal<SlotT>* signal, ConnectionId id)
        : m_signal(Ref<Signal<SlotT>>(signal)), m_id(id) {}

    ~Connection() {
        if (isConnected()) {
            disconnect();
        }
    }

    Connection(Connection&& other) noexcept {
        m_signal = other.m_signal;
        m_id = other.m_id;
        other.m_signal.release();
        other.m_id = INVALID_CONNECTION_ID;
    }

    Connection& operator=(Connection&& other) noexcept {
        if (this != &other) {
            if (isConnected()) {
                disconnect();
            }
            m_signal = other.m_signal;
            m_id = other.m_id;
            other.m_signal.release();
            other.m_id = INVALID_CONNECTION_ID;
        }
        return *this;
    }

    void disconnect() {
        if (m_signal) {
            m_signal->disconnect(m_id);
            m_signal.release();
            m_id = INVALID_CONNECTION_ID;
        }
    }

    bool isConnected() const {
        return m_id != INVALID_CONNECTION_ID;
    }

private:
    Ref<Signal<SlotT>> m_signal;
    ConnectionId m_id{INVALID_CONNECTION_ID};
};

MOE_END_NAMESPACE