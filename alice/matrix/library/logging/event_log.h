#pragma once

#include <alice/cuttlefish/library/logging/event_log.h>

#include <alice/matrix/library/logging/events/events.ev.pb.h>

namespace NMatrix {

using TLogger = NAlice::NCuttlefish::TLogger;
using TLogFrame = NAlice::NCuttlefish::TLogFrame;
using TLogFramePtr = NAlice::NCuttlefish::TLogFramePtr;

inline TLogger& GetLogger() {
    return TLogger::GetInstance();
}

inline TLogFramePtr SpawnLogFrame(bool needAlwaysSafeAdd = false) {
    return GetLogger().SpawnFrame(needAlwaysSafeAdd);
}

} // namespace NMatrix
