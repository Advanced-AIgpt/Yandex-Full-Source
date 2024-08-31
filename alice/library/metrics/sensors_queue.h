#pragma once

#include <library/cpp/monlib/metrics/labels.h>

#include <util/datetime/base.h>
#include <util/generic/vector.h>
#include <util/system/mutex.h>

namespace NAlice::NMetrics {
struct TMetric {
    NMonitoring::TLabels Labels;
    i64 Value = 0;
    TInstant Time;
};

class TSensorsQueue {
public:
    TSensorsQueue() = default;

    void Add(TMetric&& metric);

    TVector<TMetric> StealMetrics();

private:
    TVector<TMetric> Metrics;
    TMutex Mutex;
};
}
