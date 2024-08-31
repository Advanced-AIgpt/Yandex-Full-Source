#include "utils.h"

#include <alice/library/metrics/names.h>

#include <util/string/cast.h>

namespace NAlice::NMegamind {

void IncErrorOnTestIds(NMetrics::ISensors& sensors,
                       const google::protobuf::RepeatedField<google::protobuf::int64>& testIds,
                       ETestIdErrorType errorType, const NMonitoring::TLabels& labels) {
    for (const auto& testId : testIds) {
        NMonitoring::TLabels fullLabels{labels};
        fullLabels.Add(NSignal::TEST_ID, ToString(testId));
        fullLabels.Add(NSignal::NAME, NSignal::TEST_IDS_ERRORS_PER_SECOND);
        fullLabels.Add(NSignal::ERROR_TYPE, ToString(errorType));
        sensors.IncRate(fullLabels);
    }
}

NMonitoring::TLabels CreateSignalLabels(const TRequestCtx& requestCtx, TStringBuf signalName) {
    NMonitoring::TLabels signal;
    signal.Add("name", signalName);
    signal.Add("protocol", "apphost");
    signal.Add("node", requestCtx.NodeLocation());
    return signal;
}

} // namespace NAlice::NMegamind
