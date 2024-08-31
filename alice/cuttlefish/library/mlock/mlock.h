#pragma once

#include <alice/cuttlefish/library/logging/event_log.h>

#include <util/generic/function.h>
#include <util/generic/strbuf.h>

namespace NVoice {

// Return false if mlock is distabled or failed
// Report mlock state to solomon metrics
bool TryMlockAndReport(
    bool needMlock,
    std::function<void()> onDisabled = [](){},
    std::function<void(const TString&)> onError = [](const TStringBuf&){}
) ;
// Report mlock state to solomon metrics
// Write mlock disabled warning and mlock failed error to logFrame
bool TryMlockAndReport(
    bool needMlock,
    NAlice::NCuttlefish::TLogFramePtr logFrame
) ;

};
