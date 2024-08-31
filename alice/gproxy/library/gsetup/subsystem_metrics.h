#pragma once

#include <alice/cuttlefish/library/metrics/metrics.h>

namespace NGProxy {


class TMetricsSubsystem {
public:
    TMetricsSubsystem() = default;

public: /* subsystem api */
    void Init();

    inline void Wait() { }

    inline void Stop() { }

public: /* metrics api */
    NVoice::NMetrics::TScopeMetrics BeginGrpcScope() {
        NVoice::NMetrics::TMetrics& metrics = NVoice::NMetrics::TMetrics::Instance();
        return metrics.BeginScope(GrpcLabels, NVoice::NMetrics::EMetricsBackend::Golovan);
    }

    NVoice::NMetrics::TScopeMetrics BeginAppHostScope() {
        NVoice::NMetrics::TMetrics& metrics = NVoice::NMetrics::TMetrics::Instance();
        return metrics.BeginScope(AppHostLabels, NVoice::NMetrics::EMetricsBackend::Golovan);
    }

private:
    NVoice::NMetrics::TClientInfo GrpcLabels;
    NVoice::NMetrics::TClientInfo AppHostLabels;
};


}   // namespace NGProxy
