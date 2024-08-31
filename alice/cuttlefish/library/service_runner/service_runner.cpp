#include "service_runner.h"

#include <util/string/builder.h>

void NAlice::InitSolomon(TStringBuf serviceName, bool maskHost) {
    NVoice::NMetrics::TMetrics& metrics = NVoice::NMetrics::TMetrics::Instance();
    NVoice::NMetrics::TAggregationRules rules;
    metrics.SetBackend(
        NVoice::NMetrics::EMetricsBackend::Solomon,
        MakeHolder<NVoice::NMetrics::TSolomonBackend>(
            rules,
            NVoice::NMetrics::MakeMillisBuckets(),
            serviceName,
            maskHost
        )
    );
}
