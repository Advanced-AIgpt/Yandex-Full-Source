#include "walker_util.h"

namespace NAlice::NMegamind {

void UpdateMetricsData(TRequestTimeMetrics& rtm, ILightWalkerRequestCtx& wCtx) {
    if (const auto* startTime = wCtx.RequestCtx().StageTimers().Find(TS_STAGE_START_REQUEST); startTime != nullptr) {
        rtm.SetStartTime(*startTime);
    }
    const auto skr = wCtx.Ctx().SpeechKitRequest();
    rtm.SetSkrInfo(skr.ClientInfo().Name, skr.Path());
}

} // namespace NAlice::NMegamind
