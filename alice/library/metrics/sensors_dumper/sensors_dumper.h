#pragma once

#include <alice/library/metrics/service.h>
#include <alice/library/metrics/unistat.h>

#include <library/cpp/monlib/metrics/metric_registry.h>

#include <util/generic/ptr.h>


namespace NAlice {

class TSensorsDumper {
public:
    TSensorsDumper(NMonitoring::TMetricRegistry& solomonSensors);

    TString Dump(TStringBuf format);

private:
    const NMonitoring::TMetricRegistry& SolomonSensors;
};

} // namespace NAlice
