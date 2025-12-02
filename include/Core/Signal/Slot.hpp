#pragma once

#include "Core/Common.hpp"
#include "Core/Meta/Feature.hpp"
#include "Core/Sync.hpp"

MOE_BEGIN_NAMESPACE

template<typename Derived>
struct Slot
    : public MutexSynchronized<Derived>,
      public Meta::NonCopyable<Slot<Derived>> {
public:
    virtual ~Slot() = default;

    Slot() = default;
    Slot(Slot&&) = default;
    Slot& operator=(Slot&&) = default;

    template<typename... Args>
    void _signal(Args&&... args) {
        this->withLock([&]() {
            static_cast<Derived*>(this)->signal(std::forward<Args>(args)...);
        });
    }
};

MOE_END_NAMESPACE