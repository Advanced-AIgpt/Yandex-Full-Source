#pragma once

#include "backend.h"
#include "settings.h"


namespace NVoice {
namespace NMetrics {


class TDummySensorImpl;


class TDummyBackend : public IBackend {
public:
    TDummyBackend(TAggregationRules rules, const NMonitoring::TBucketBounds& buckets, bool logging=true);

    ~TDummyBackend();

    ISensorPtr CreateAbsoluteSensor(const TString& sensorName, TStringBuf code, TLabels labels) override;

    ISensorPtr CreateRateSensor(const TString& sensorName, TStringBuf code, TLabels labels) override;

    ISensorPtr CreateHistogramSensor(const TString& sensorName, TStringBuf code, TLabels labels) override;

    TString    BuildSensorName(TStringBuf scope, TStringBuf sensor, TStringBuf code, TStringBuf backend, TLabels labels) override;

    bool       SerializeMetrics(IOutputStream& stream, EOutputFormat format) override;

    void       Reset() override { }

private:    /* methods */
    ISensorPtr CreateSensor(const TString& sensorName, TStringBuf code, TLabels labels, TStringBuf type, bool logging);

private:    /* data */
    NMonitoring::TBucketBounds  Buckets_;
    TVector<TIntrusivePtr<TDummySensorImpl>> Sensors_;
    bool Logging { true };
};  // class TDummuyBackend


}   // namespace NMetrics
}   // namespace NVoice
