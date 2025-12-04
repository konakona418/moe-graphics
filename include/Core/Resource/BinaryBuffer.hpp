#pragma once

#include "Core/Common.hpp"
#include "Core/RefCounted.hpp"

MOE_BEGIN_NAMESPACE

struct BinaryBuffer : public RefCounted<BinaryBuffer> {
public:
    BinaryBuffer(Vector<uint8_t>&& data, StringView mimeType = "")
        : m_data(std::move(data)), m_mimeType(mimeType) {}

    const uint8_t* data() const { return m_data.data(); }

    size_t size() const { return m_data.size(); }

    String mimeType() const { return m_mimeType; }

    Span<const uint8_t> asSpan() const {
        return Span<const uint8_t>(m_data.data(), m_data.size());
    }

    Span<uint8_t> asMutableSpan() {
        return Span<uint8_t>(m_data.data(), m_data.size());
    }

private:
    Vector<uint8_t> m_data;
    String m_mimeType;
};

MOE_END_NAMESPACE