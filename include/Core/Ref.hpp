#pragma once

#include "Core/Common.hpp"
#include "Core/Meta/RefCounted.hpp"
#include "Core/Meta/TypeTraits.hpp"


MOE_BEGIN_NAMESPACE

template<typename T, typename = Meta::EnableIfT<Meta::IsRefCounted<T>>>
struct Ref {
public:
    Ref() = default;

    explicit Ref(T* ptr)
        : m_ptr(ptr) {
        retain();
    }

    Ref(const Ref& other)
        : m_ptr(other.m_ptr) {
        retain();
    }

    Ref(Ref&& other) noexcept
        : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    ~Ref() {
        release();
    }

    Ref& operator=(const Ref& other) {
        if (this != &other) {
            release();
            m_ptr = other.m_ptr;
            retain();
        }
        return *this;
    }

    Ref& operator=(Ref&& other) noexcept {
        if (this != &other) {
            release();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    T* get() {
        MOE_ASSERT(m_ptr != nullptr, "dereferencing null Ref pointer");
        return m_ptr;
    }

    const T* get() const {
        MOE_ASSERT(m_ptr != nullptr, "dereferencing null Ref pointer");
        return m_ptr;
    }

    T* operator->() {
        MOE_ASSERT(m_ptr != nullptr, "dereferencing null Ref pointer");
        return m_ptr;
    }

    const T* operator->() const {
        MOE_ASSERT(m_ptr != nullptr, "dereferencing null Ref pointer");
        return m_ptr;
    }

    bool operator==(const Ref& other) const { return m_ptr == other.m_ptr; }
    bool operator!=(const Ref& other) const { return m_ptr != other.m_ptr; }

    void reset(T* ptr = nullptr) {
        if (m_ptr != ptr) {
            release();
            m_ptr = ptr;
            retain();
        }
    }

    void swap(Ref& other) noexcept {
        std::swap(m_ptr, other.m_ptr);
    }

    explicit operator bool() const { return m_ptr != nullptr; }

    void retain() {
        if (m_ptr) {
            m_ptr->retain();
        }
    }

    void release() {
        if (m_ptr) {
            m_ptr->release();
            m_ptr = nullptr;
        }
    }

private:
    T* m_ptr{nullptr};
};

MOE_END_NAMESPACE
