#pragma once

#include <chrono>
#include <util/system/types.h>

namespace NAlice {

inline i64 SystemTimeNowMillis() {
    const auto timePoint = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count();
}

} // namespace NAlice
