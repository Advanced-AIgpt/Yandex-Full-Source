#pragma once

#include <alice/cuttlefish/library/metrics/metrics.h>

namespace NAlice::NTtsCacheProxy {

    class TSourceMetrics : public NVoice::NMetrics::TSourceMetrics {
    public:
        TSourceMetrics(TStringBuf sourceName)
            : NVoice::NMetrics::TSourceMetrics(
                sourceName,
                MakeEmptyClientInfo(),
                NVoice::NMetrics::EMetricsBackend::Solomon
            )
        {}
    };

} // NAlice::NTtsCacheProxy
