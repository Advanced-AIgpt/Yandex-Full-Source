#pragma once

#include <voicetech/library/logger/logger.h>

namespace NAlice::NCuttlefish {

using TLogger = NVoicetech::TLogger;
using TLogFrame = NVoicetech::TSelfFlushEventLogFrame;
using TLogFramePtr = NVoicetech::TSelfFlushEventLogFramePtr;

inline TLogger& GetLogger() {
    return TLogger::GetInstance();
}

inline TLogFramePtr SpawnLogFrame(bool needAlwaysSafeAdd = false) {
    return GetLogger().SpawnFrame(needAlwaysSafeAdd);
}

}  // namespace NAlice::NCuttlefish
