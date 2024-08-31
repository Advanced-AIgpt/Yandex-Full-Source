#pragma once

#include <alice/megamind/library/requestctx/requestctx.h>

#include <alice/library/metrics/sensors.h>
#include <alice/library/proto/protobuf.h>

#include <library/cpp/monlib/metrics/labels.h>

namespace NAlice::NMegamind {

enum class ETestIdErrorType {
    HTTP_ERROR /* "http_error" */,
    SCENARIO_ERROR /* "scenario_error" */,
};

void IncErrorOnTestIds(NMetrics::ISensors& sensors,
                       const google::protobuf::RepeatedField<google::protobuf::int64>& testIds,
                       ETestIdErrorType errorType, const NMonitoring::TLabels& labels);

NMonitoring::TLabels CreateSignalLabels(const TRequestCtx& requestCtx, TStringBuf signalName);

} // namespace NAlice::NMegamind
