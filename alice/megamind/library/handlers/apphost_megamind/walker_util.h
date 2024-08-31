#pragma once

#include <alice/megamind/library/handlers/utils/sensors.h>
#include <alice/megamind/library/walker/requestctx.h>

namespace NAlice::NMegamind {

void UpdateMetricsData(TRequestTimeMetrics& rtm, ILightWalkerRequestCtx& wCtx);

} // namespace NAlice::NMegamind
