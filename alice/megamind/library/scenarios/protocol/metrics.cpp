#include "metrics.h"

#include <alice/library/metrics/sensors.h>
#include <alice/library/metrics/util.h>

namespace NAlice {

TProtocolScenarioMetrics::TProtocolScenarioMetrics(NMetrics::ISensors& sensors, const TString& scenarioName,
                                                   bool isSmartSpeaker,
                                                   const NScenarios::TScenarioBaseRequest_ERequestSourceType& requestSourceType)
    : Sensors{sensors}
    , LabelsGenerator(scenarioName, isSmartSpeaker, requestSourceType) {
}

void TProtocolScenarioMetrics::OnStartRequest() {
    Sensors.IncRate(LabelsGenerator.RequestType(/* hedged= */ false));
}

void TProtocolScenarioMetrics::OnFinishRequest(const NHttpFetcher::TResponseStats& stats) {
    const bool hedged = false;
    Sensors.IncRate(LabelsGenerator.ResponseType(hedged));
    Sensors.AddHistogram(LabelsGenerator.ResponseTime(hedged, /* getNext= */ false), stats.Duration.MilliSeconds(),
                         NMetrics::TIME_INTERVALS);
    if (LabelsGenerator.IsRequestSourceTypeGetNext()) {
        Sensors.AddHistogram(LabelsGenerator.ResponseTime(hedged, /* getNext= */ true), stats.Duration.MilliSeconds(),
                             NMetrics::TIME_INTERVALS);
        Sensors.IncRate(LabelsGenerator.ResponseStatus(ToString(stats.Result), hedged, /* getNext= */ true));
    }
    Sensors.IncRate(LabelsGenerator.ResponseHttpCode(ToString(stats.HttpCode), hedged));
    Sensors.IncRate(LabelsGenerator.ResponseStatus(ToString(stats.Result), hedged, /* getNext= */ false));
    Sensors.AddHistogram(LabelsGenerator.ResponseSize(hedged), stats.ResponseSize, NMetrics::SIZE_INTERVALS);
}

void TProtocolScenarioMetrics::OnStartHedgedRequest(const NHttpFetcher::TRequestStats& stats) {
    Sensors.IncRate(LabelsGenerator.RequestType(/* hedged= */ true));
    Sensors.AddRate(LabelsGenerator.RequestSize(/* hedged= */ true, NMonitoring::EMetricType::RATE),
                    stats.RequestSize);
    Sensors.AddHistogram(LabelsGenerator.RequestSize(/* hedged= */ true, NMonitoring::EMetricType::HIST),
                         stats.RequestSize, NMetrics::SIZE_INTERVALS);
}

void TProtocolScenarioMetrics::OnFinishHedgedRequest(const NHttpFetcher::TResponseStats& stats) {
    Sensors.IncRate(LabelsGenerator.ResponseStatus(ToString(stats.Result), /* hedged= */ true, /* getNext= */ false));
}

} // namespace NAlice
