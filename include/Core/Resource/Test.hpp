#pragma once

#include "Core/Common.hpp"
#include "Core/Meta/Generator.hpp"

MOE_BEGIN_NAMESPACE

template<typename T, bool FailBehavior = false>
struct TestGenerator {
public:
    using value_type = T;
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

MOE_END_NAMESPACE