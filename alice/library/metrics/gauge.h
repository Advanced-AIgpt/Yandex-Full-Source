#pragma once

#include "sensors.h"

#include <util/generic/strbuf.h>

namespace NAlice {

class TOngoingRequestsCounter final {
public:
    TOngoingRequestsCounter(NMetrics::ISensors& sensors, TStringBuf name = "ongoing_requests");
    TOngoingRequestsCounter(NMetrics::ISensors& sensors, NMonitoring::TLabels labels);
    ~TOngoingRequestsCounter();

private:
    NMetrics::ISensors& Sensors_;
    NMonitoring::TLabels Labels_;
};

} // namespace NAlice
