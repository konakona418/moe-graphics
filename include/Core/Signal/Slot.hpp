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

    Slot(Slot&& other) noexcept = default;
    Slot& operator=(Slot&& other) noexcept = default;

    template<typename... Args>
    void _invoke(Args&&... args) {
        this->withLock([&]() {
            static_cast<Derived*>(this)->invoke(std::forward<Args>(args)...);
        });
    }
};

MOE_END_NAMESPACE