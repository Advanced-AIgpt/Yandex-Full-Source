#pragma once

#include "fwd.h"
#include "sensors_queue.h"

#include <library/cpp/monlib/metrics/labels.h>

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/system/types.h>

#include <utility>

namespace NAlice::NMetrics {

class ISensors {
public:
    virtual ~ISensors() = default;

    // Helpers for manipulating counters.

    void AddRate(const NMonitoring::TLabels& labels, i32 delta);
    virtual void AddRate(NMonitoring::TLabels&& labels, i32 delta) = 0;

    void IncRate(const NMonitoring::TLabels& labels);
    virtual void IncRate(NMonitoring::TLabels&& labels) = 0;

    void AddHistogram(const NMonitoring::TLabels& labels, ui64 value, const TVector<double>& bins);
    virtual void AddHistogram(NMonitoring::TLabels&& labels, ui64 value, const TVector<double>& bins) = 0;

    void AddIntGauge(const NMonitoring::TLabels& labels, i64 value);
    virtual void AddIntGauge(NMonitoring::TLabels&& labels, i64 value) = 0;

    void SetIntGauge(const NMonitoring::TLabels& labels, i64 value);
    virtual void SetIntGauge(NMonitoring::TLabels&& labels, i64 value) = 0;
};

// Wrapper to add common labels to all metrics
class TSensorsWrapper : public ISensors {
public:
    TSensorsWrapper(const NMonitoring::TLabels& labels, ISensors& baseSensors);

    void AddRate(NMonitoring::TLabels&& labels, i32 delta) override;

    void IncRate(NMonitoring::TLabels&& labels) override;

    void AddHistogram(NMonitoring::TLabels&& labels, ui64 value, const TVector<double>& bins) override;

    void AddIntGauge(NMonitoring::TLabels&& labels, i64 value) override;

    void SetIntGauge(NMonitoring::TLabels&& labels, i64 value) override;

private:
    void EnrichLabels(NMonitoring::TLabels& labels);

private:
    TVector<std::pair<TString, TString> > CommonLabels_;
    ISensors& BaseSensors_;
};

} // 
