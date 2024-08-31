#pragma once

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/library/metrics/fwd.h>
#include <alice/library/metrics/names.h>

#include <library/cpp/monlib/metrics/labels.h>

#include <util/generic/strbuf.h>

namespace NAlice {

class TProtocolScenarioMetrics : public NHttpFetcher::IMetrics {
public:
    TProtocolScenarioMetrics(NMetrics::ISensors& sensors, const TString& scenarioName, bool isSmartSpeaker, const NScenarios::TScenarioBaseRequest_ERequestSourceType& requestSourceType);
    void OnStartRequest() override;
    void OnFinishRequest(const NHttpFetcher::TResponseStats& stats) override;
    void OnStartHedgedRequest(const NHttpFetcher::TRequestStats& stats) override;
    void OnFinishHedgedRequest(const NHttpFetcher::TResponseStats& stats) override;

private:
    NMetrics::ISensors& Sensors;
    NSignal::TProtocolScenarioHttpLabelsGenerator LabelsGenerator;
};

} // namespace NAlice
