#include "stage_timers.h"

#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/sensors/utils.h>

#include <alice/library/metrics/util.h>

namespace NAlice::NMegamind {

bool TStageTimers::RegisterAndSignal(
    TRequestCtx& ctx,
    TStringBuf stageName,
    TStringBuf startStageName,
    NMetrics::ISensors& sensors) 
{
    const auto* startTime = Find(startStageName);
    if (!startTime) {
        return false;
    }

    const auto duration = (Register(stageName) - *startTime).MilliSeconds();
    const auto labels = CreateSignalLabels(ctx, stageName);
    sensors.AddHistogram(labels, duration, NAlice::NMetrics::TIME_INTERVALS);

    return true;
}

TInstant TStageTimers::Register(TStringBuf name) {
    const auto time = TInstant::Now();
    Storage_[name] = time;
    Upload(name, time);
    return time;
}

const TInstant* TStageTimers::Find(TStringBuf name) const {
    return Storage_.FindPtr(name);
}

} // namespace NAlice::NMegamind
