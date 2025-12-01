#pragma once

#include "Core/Common.hpp"

MOE_BEGIN_NAMESPACE

using ConnectionId = uint32_t;
using AtomicConnectionId = std::atomic_uint32_t;
static constexpr ConnectionId INVALID_CONNECTION_ID =
        std::numeric_limits<uint32_t>::max();

MOE_END_NAMESPACE