#pragma once

#include "Core/Common.hpp"

MOE_BEGIN_NAMESPACE

struct RefCounted {
public:
    void retain() {
        ++m_refCount;
    }

    void release() {
        MOE_ASSERT(m_refCount > 0, "release called on object with zero ref count");
        --m_refCount;
        if (m_refCount == 0) {
            delete this;
        }
    }

private:
    size_t m_refCount{0};
};

struct AtomicRefCounted {
public:
    void retain() {
        m_refCount.fetch_add(1, std::memory_order_relaxed);
    }

    void release() {
        size_t oldCount = m_refCount.fetch_sub(1, std::memory_order_acq_rel);
        MOE_ASSERT(oldCount > 0, "release called on object with zero ref count");
        if (oldCount == 1) {
            delete this;
        }
    }

private:
    std::atomic<size_t> m_refCount{0};
};

MOE_END_NAMESPACE