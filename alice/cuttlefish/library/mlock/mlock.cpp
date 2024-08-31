
#include "mlock.h"

#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>

#include <util/system/mlock.h>

namespace NVoice {

bool TryMlockAndReport(
    bool needMlock,
    std::function<void()> onDisabled,
    std::function<void(const TString&)> onError
) {
    NVoice::NMetrics::TMetrics& metrics = NVoice::NMetrics::TMetrics::Instance();
    if (needMlock) {
        try {
            LockAllMemory(ELockAllMemoryFlag::LockCurrentMemory | ELockAllMemoryFlag::LockFutureMemory);
            metrics.PushAbs(1, "system", "memory_locked", "success", "", {}, NVoice::NMetrics::EMetricsBackend::Solomon);
            return true;
        } catch (...) {
            metrics.PushAbs(1, "system", "memory_locked", "fail", "", {}, NVoice::NMetrics::EMetricsBackend::Solomon);
            try {
                onError(CurrentExceptionMessage());
            } catch (...) {
                // ¯\_(ツ)_/¯
            }

            return false;
        }
    } else {
        metrics.PushAbs(1, "system", "memory_locked", "disable", "", {}, NVoice::NMetrics::EMetricsBackend::Solomon);
        try {
            onDisabled();
        } catch (...) {
            // ¯\_(ツ)_/¯
        }

        return false;
    }
}

bool TryMlockAndReport(
    bool needMlock,
    NAlice::NCuttlefish::TLogFramePtr logFrame
) {
    return TryMlockAndReport(
        needMlock,
        [&logFrame]() {
            logFrame->LogEvent(NEvClass::MlockDisabled());
        },
        [&logFrame](const TString& error) {
            logFrame->LogEvent(NEvClass::MlockError(error));
        }
    );
}

} // NVoice
