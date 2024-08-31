#pragma once

#include "fwd.h"

#include <library/cpp/monlib/service/pages/mon_page.h>

#include <util/generic/string.h>

namespace NAlice::NMetrics {

class TGolovanCountersPage : public NMonitoring::IMonPage {
public:
    TGolovanCountersPage(const TString& path, NMonitoring::TMetricRegistry& sensors);

    TString DumpSensors();

    void Output(NMonitoring::IMonHttpRequest& request) override;

private:
    NMonitoring::TMetricRegistry& Sensors;
};

} // namespace NAlice::NMetrics
