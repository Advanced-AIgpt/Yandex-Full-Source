#include "mlock.h"

#include <alice/matrix/library/metrics/metrics.h>

#include <util/system/mlock.h>


namespace NMatrix {

bool TryMlockAndReport(
    bool needMlock,
    TLogFramePtr logFrame
) {
    TSourceMetrics metrics("mlock");
    if (needMlock) {
        try {
            LockAllMemory(ELockAllMemoryFlag::LockCurrentMemory | ELockAllMemoryFlag::LockFutureMemory);

            logFrame->LogEvent(NEvClass::TMatrixServerMemoryLocked());
            metrics.IncGauge("status", "ok");

            return true;
        } catch (...) {
            logFrame->LogEvent(NEvClass::TMatrixServerMemoryLockError(CurrentExceptionMessage()));
            metrics.IncGauge("status", "error");

            return false;
        }
    } else {
        logFrame->LogEvent(NEvClass::TMatrixServerMemoryLockDisabled());
        metrics.IncGauge("status", "disabled");

        return false;
    }
}

} // namespace NMatrix
