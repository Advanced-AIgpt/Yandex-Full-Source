#pragma once

#include <util/datetime/base.h>

#include <cstdint>

namespace NAlice::NMegamind {

// If timeoutUs is 0, false will be returned
inline bool IsTimeoutExceeded(const ui64 eventTimeMs, const ui64 timeoutMs,
                              const ui64 serverTimeMs = TInstant::Now().MilliSeconds()) {
    return timeoutMs > 0 && serverTimeMs > eventTimeMs && (serverTimeMs - eventTimeMs) > timeoutMs;
}

} // namespace NAlice::NMegamind
