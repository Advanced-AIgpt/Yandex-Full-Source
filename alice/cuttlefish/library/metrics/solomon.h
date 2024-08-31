#pragma once

#include "backend.h"
#include "settings.h"

#include <library/cpp/monlib/metrics/metric_registry.h>
#include <util/generic/hash_set.h>


namespace NVoice {
namespace NMetrics {

class TSolomonSensor;

using TSolomonSensorPtr = TIntrusivePtr<TSolomonSensor>;


class TSolomonBackend : public IBackend {
public:
    TSolomonBackend(TAggregationRules rules, const NMonitoring::TBucketBounds& buckets, TStringBuf service = "proxy", bool maskHost = true);

    ~TSolomonBackend();

    ISensorPtr CreateAbsoluteSensor(const TString& sensorName, TStringBuf code, TLabels labels) override;

    ISensorPtr CreateRateSensor(const TString& sensorName, TStringBuf code, TLabels labels) override;

    ISensorPtr CreateHistogramSensor(const TString& sensorName, TStringBuf code, TLabels labels) override;

    TString    BuildSensorName(TStringBuf scope, TStringBuf sensor, TStringBuf code, TStringBuf backend, TLabels labels) override;

    bool       SerializeMetrics(IOutputStream& stream, EOutputFormat format) override;

    void       Reset() override;

private:    /* methods */
    void FillLabels(NMonitoring::TLabels& solomonLabels, const TString& sensorName, TStringBuf code, const TLabels& labels) const;

    TString GetHostCType() const;

private:    /* methods */
    NMonitoring::TBucketBounds      Buckets_;
    NMonitoring::TMetricRegistry    Registry_;
    bool MaskHostname_  { true };
    TString CType_      { "dev" };
};  // class TSolomonBackend


}   // namespace NMetrics
}   // namespace NVoice
