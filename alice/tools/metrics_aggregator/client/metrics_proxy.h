#pragma once

#include <alice/tools/metrics_aggregator/library/util.h>

#include <alice/bass/libs/scheduler/cache.h>

#include <alice/library/metrics/sensors.h>
#include <alice/library/metrics/sensors_dumper/sensors_dumper.h>

#include <library/cpp/monlib/metrics/labels.h>

#include <util/system/rwlock.h>

namespace NMetricsAggregator {

class TMetricsProxy final : public NAlice::NMetrics::ISensors, NNonCopyable::TNonCopyable {
public:
    TMetricsProxy(int port);

    void AddRate(NMonitoring::TLabels&& labels, i32 value) override;

    void IncRate(NMonitoring::TLabels&& labels) override;

    void AddHistogram(NMonitoring::TLabels&& labels, ui64 value, const TVector<double>& bins = BUCKETS) override;

    void AddIntGauge(NMonitoring::TLabels&& labels, i64 value) override;

    void SetIntGauge(NMonitoring::TLabels&& labels, i64 value) override;

private:
    TDuration PushMetrics();

private:
    TString BatchUrl_;

    TAtomicSharedPtr<NMonitoring::TMetricRegistry> SolomonSensors_;
    NAlice::TSensorsDumper SensorsDumper_;

    NBASS::TCacheManager Scheduler_;
};

} // namespace NMetricsAggregator
