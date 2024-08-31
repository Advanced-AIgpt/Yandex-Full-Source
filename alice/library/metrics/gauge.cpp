#include "gauge.h"

namespace NAlice {

TOngoingRequestsCounter::TOngoingRequestsCounter(NMetrics::ISensors& sensors, TStringBuf name)
    : TOngoingRequestsCounter{sensors, NMonitoring::TLabels{{"name", name}}}
{
}

TOngoingRequestsCounter::TOngoingRequestsCounter(NMetrics::ISensors& sensors, NMonitoring::TLabels labels)
    : Sensors_{sensors}
    , Labels_{std::move(labels)}
{
    Sensors_.AddIntGauge(Labels_, 1);
}

TOngoingRequestsCounter::~TOngoingRequestsCounter() {
    try {
        Sensors_.AddIntGauge(Labels_, -1);
    } catch (...) {
    }
}

} // namespace NAlice
