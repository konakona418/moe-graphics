#pragma once

#include "Core/Common.hpp"
#include "Core/Meta/Feature.hpp"
#include "Core/Sync.hpp"

#include "Core/Signal/Common.hpp"
#include "Core/Signal/Connection.hpp"

#include "Core/RefCounted.hpp"

MOE_BEGIN_NAMESPACE

template<typename SlotT>
struct Signal
    : public AtomicRefCounted<Signal<SlotT>>,
      public Meta::NonCopyable<Signal<SlotT>>,
      public RwSynchronized<Signal<SlotT>> {
public:
    Signal() = default;
    ~Signal() = default;

    Connection<SlotT> connect(SlotT&& slot) {
        ConnectionId id = m_connectionId.fetch_add(1);
        if (m_emittingCount.load(std::memory_order_relaxed) > 0) {
            {
                std::lock_guard<std::mutex> lk(m_pendingConnectsMutex);
                m_pendingConnects.emplace_back(id, std::move(slot));
            }
            return Connection<SlotT>(this, id);
        }

        this->withWriteLock([&]() {
            m_slots.emplace(id, std::move(slot));
        });
        return Connection<SlotT>(this, id);
    }

    void disconnect(ConnectionId id) {
        if (m_emittingCount.load(std::memory_order_relaxed) > 0) {
            {
                std::lock_guard<std::mutex> lk(m_pendingDisconnectsMutex);
                m_pendingDisconnects.push_back(id);
            }
            return;
        }

        this->withWriteLock([&]() {
            m_slots.erase(id);
        });
    }

    template<typename... Args>
    void emit(Args&&... args) {
        m_emittingCount.fetch_add(1, std::memory_order_relaxed);
        this->withReadLock([&]() {
            for (auto& [id, slot]: m_slots) {
                slot._invoke(std::forward<Args>(args)...);
            }
        });
        m_emittingCount.fetch_sub(1, std::memory_order_relaxed);

        if (m_emittingCount.load(std::memory_order_relaxed) == 0) {
            handlePendingOps();
        }
    }

    void handlePendingOps() {
        std::lock_guard<std::mutex> lk(m_handlePendingMutex);
        Vector<ConnectionId> pendingDisconnects;
        {
            std::lock_guard<std::mutex> lk(m_pendingDisconnectsMutex);
            pendingDisconnects.swap(m_pendingDisconnects);
        }

        Vector<Pair<ConnectionId, SlotT>> pendingConnects;
        {
            std::lock_guard<std::mutex> lk(m_pendingConnectsMutex);
            pendingConnects.swap(m_pendingConnects);
        }

        this->withWriteLock([&]() {
            for (ConnectionId id: pendingDisconnects) {
                m_slots.erase(id);
            }
            for (auto& [id, slot]: pendingConnects) {
                m_slots.emplace(id, std::move(slot));
            }
        });
    }

private:
    AtomicConnectionId m_connectionId{0};
    UnorderedMap<ConnectionId, SlotT> m_slots;

    std::mutex m_pendingDisconnectsMutex;
    Vector<ConnectionId> m_pendingDisconnects;

    std::mutex m_pendingConnectsMutex;
    Vector<Pair<ConnectionId, SlotT>> m_pendingConnects;

    std::mutex m_handlePendingMutex;

    std::atomic_size_t m_emittingCount{0};
};

MOE_END_NAMESPACE