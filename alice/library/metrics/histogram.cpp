#include "histogram.h"

#include "sensors.h"
#include "util.h"

namespace NAlice {

// THistogramScope ------------------------------------------------------------
THistogramScope::THistogramScope(NMetrics::ISensors& sensors, NMonitoring::TLabels labels, ETimeUnit timeUnit,
                                 const TAggregateLabelsBuilder& aggregateLabelsBuilder,
                                 std::function<void(const TDuration&)> onDuration)
    : Start{std::chrono::steady_clock::now()}
    , Labels{std::move(labels)}
    , Sensors{sensors}
    , TimeUnit(timeUnit)
    , AggregateLabelsBuilder{aggregateLabelsBuilder}
    , OnDuration(onDuration)
{
}

THistogramScope::~THistogramScope() {
    const auto duration = std::chrono::steady_clock::now() - Start;
    const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);
    TDuration duration2 = TDuration::MicroSeconds(microseconds.count());

    if (OnDuration) {
        OnDuration(duration2);
    }

    ui64 value;
    switch (TimeUnit) {
        case ETimeUnit::Millis:
            value = duration2.MilliSeconds();
            break;
        case ETimeUnit::Micros:
            value = duration2.MicroSeconds();
            break;
    }
    Sensors.AddHistogram(Labels, value, NMetrics::TIME_INTERVALS);

    for (const auto& aggregatelabels : AggregateLabelsBuilder.Build(Labels)) {
        Sensors.AddHistogram(aggregatelabels, value, NMetrics::TIME_INTERVALS);
    }
}

} // namespace NAlice
