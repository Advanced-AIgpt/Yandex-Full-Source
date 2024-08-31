#pragma once

#include "aggregate_labels_builder.h"
#include "fwd.h"

#include <library/cpp/monlib/metrics/labels.h>

#include <util/datetime/base.h>

#include <chrono>

namespace NAlice {

// Creating an object of this class will result in pushing its lifespan time to histograms
class THistogramScope final {
public:
    enum class ETimeUnit {
        Millis,
        Micros
    };

    THistogramScope(NMetrics::ISensors& sensors, NMonitoring::TLabels labels, ETimeUnit timeUnit,
                    const TAggregateLabelsBuilder& aggregateLabelsBuilder={},
                    std::function<void(const TDuration&)> onDuration={});
    ~THistogramScope();

private:
    const std::chrono::time_point<std::chrono::steady_clock> Start;
    const NMonitoring::TLabels Labels;
    NMetrics::ISensors& Sensors;
    const ETimeUnit TimeUnit;
    const TAggregateLabelsBuilder AggregateLabelsBuilder;
    std::function<void(const TDuration&)> OnDuration;
};

} // namespace NAlice
