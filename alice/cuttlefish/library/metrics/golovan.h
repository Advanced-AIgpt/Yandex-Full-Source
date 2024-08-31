#pragma once

#include "backend.h"

#include <library/cpp/monlib/metrics/histogram_collector.h>
#include <library/cpp/unistat/unistat.h>


namespace NVoice {
namespace NMetrics {


class TGolovanBackend : public IBackend {
public:
    TGolovanBackend(TAggregationRules rules, const NMonitoring::TBucketBounds& buckets);

    ISensorPtr CreateAbsoluteSensor(const TString& sensorName, TStringBuf code, TLabels labels) override;

    ISensorPtr CreateRateSensor(const TString& sensorName, TStringBuf code, TLabels labels) override;

    ISensorPtr CreateHistogramSensor(const TString& sensorName, TStringBuf code, TLabels labels) override;

    TString  BuildSensorName(TStringBuf scope, TStringBuf sensor, TStringBuf code, TStringBuf backend, TLabels labels) override;

    bool     SerializeMetrics(IOutputStream& stream, EOutputFormat format) override;

    void     Reset() override;
private:
    TString BuildHoleName(TStringBuf sensorName, TStringBuf code, const TLabels& labels);

private:
    NUnistat::TIntervals Buckets;
};


}   // namespace NMetrics
}   // namespace NVoice
