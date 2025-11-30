#pragma once

#include "Core/Common.hpp"
#include "Core/Meta/Generator.hpp"

MOE_BEGIN_NAMESPACE

template<typename T, bool FailBehavior = false>
struct TestGenerator {
public:
    using value_type = T;

    TestGenerator() = default;

    TestGenerator(const T& value)
        : m_value(value) {}

    Optional<value_type> generate() {
        if constexpr (FailBehavior) {
            return std::nullopt;
        }
        return m_value;
    }

    uint64_t hashCode() const {
        return std::hash<T>()(m_value);
    }

    String paramString() const {
        return fmt::format("test_generator_{}", m_value);
    }

private:
    T m_value;
};

template<typename T, bool FailBehavior = false>
struct TestGeneratorAsync {
public:
    using value_type = T;

    TestGeneratorAsync() = default;

    TestGeneratorAsync(const T& value, std::chrono::milliseconds delay)
        : m_value(value), m_delay(delay) {}

    Optional<value_type> generate() {
        std::this_thread::sleep_for(m_delay);
        if constexpr (FailBehavior) {
            return std::nullopt;
        }
        return m_value;
    }

    uint64_t hashCode() const {
        return std::hash<T>()(m_value);
    }

    String paramString() const {
        return fmt::format("test_generator_async_{}", m_value);
    }

private:
    T m_value;
    std::chrono::milliseconds m_delay{0};
};

MOE_END_NAMESPACE