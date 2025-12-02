#pragma once

#include "Core/Common.hpp"
#include "Core/Meta/Feature.hpp"
#include "Core/Sync.hpp"

MOE_BEGIN_NAMESPACE

template<typename Derived>
struct Slot : public Meta::NonCopyable<Slot<Derived>> {
public:
    virtual ~Slot() = default;

    Slot() = default;
    Slot(Slot&&) = default;
    Slot& operator=(Slot&&) = default;

    template<typename... Args>
    void _signal(Args&&... args) {
        static_cast<Derived*>(this)->signal(std::forward<Args>(args)...);
    }
};

template<typename Derived>
struct SyncSlot
    : public MutexSynchronized<Derived>,
      public Meta::NonCopyable<SyncSlot<Derived>> {
public:
    virtual ~SyncSlot() = default;

    SyncSlot() = default;
    SyncSlot(SyncSlot&&) = default;
    SyncSlot& operator=(SyncSlot&&) = default;

    template<typename... Args>
    void _signal(Args&&... args) {
        this->withLock([&]() {
            static_cast<Derived*>(this)->signal(std::forward<Args>(args)...);
        });
    }
};

MOE_END_NAMESPACE