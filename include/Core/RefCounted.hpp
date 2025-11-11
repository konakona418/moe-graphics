#pragma once

#include "Core/Common.hpp"

MOE_BEGIN_NAMESPACE

template<typename T>
struct RefCounted {
public:
    // ! fixme: use CRTP to avoid virtual overhead
    virtual ~RefCounted() = default;

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

    size_t getRefCount() const {
        return m_refCount;
    }

private:
    size_t m_refCount{0};
};

template<typename T>
struct AtomicRefCounted {
public:
    virtual ~AtomicRefCounted() = default;

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

    size_t getRefCount() const {
        return m_refCount.load(std::memory_order_relaxed);
    }

private:
    std::atomic<size_t> m_refCount{0};
};

MOE_END_NAMESPACE