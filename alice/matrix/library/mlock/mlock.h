#pragma once

#include <alice/matrix/library/logging/event_log.h>

namespace NMatrix {

// Report mlock state to solomon metrics
// Write mlock disabled warning and mlock failed error to logFrame
bool TryMlockAndReport(
    bool needMlock,
    TLogFramePtr logFrame
) ;

} // namespace NMatrix
