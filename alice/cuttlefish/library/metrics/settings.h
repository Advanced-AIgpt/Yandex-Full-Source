#pragma once

#include "aggregation.h"

#include <library/cpp/monlib/metrics/histogram_collector.h>


namespace NVoice {
namespace NMetrics {


enum class EMetricsBackend : int {
    Dummy,
    Golovan,
    Solomon,

    Max
};


struct TMetricsSettings {
public:
    TMetricsSettings& SetAggregationRules(const TAggregationRules& rules) {
        Rules_ = rules;
        return *this;
    }

    const TAggregationRules& AggregationRules() const {
        return Rules_;
    }

    TAggregationRules& AggregationRules() {
        return Rules_;
    }

    TMetricsSettings& SetBucketSettings(const NMonitoring::TBucketBounds& buckets) {
        BucketBounds_ = buckets;
        return *this;
    }

    const NMonitoring::TBucketBounds& BucketBounds() const {
        return BucketBounds_;
    }

    NMonitoring::TBucketBounds& BucketBounds() {
        return BucketBounds_;
    }


    TMetricsSettings& MaskHostname(bool value = true) {
        MaskHostname_ = value;
        return *this;
    }

private:
    TAggregationRules    Rules_;
    NMonitoring::TBucketBounds BucketBounds_;
    bool MaskHostname_ { true };
};


}   // namespace NMetrics
}   // namespace NVoice
