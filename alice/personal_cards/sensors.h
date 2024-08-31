#pragma once

#include <infra/libs/sensors/sensor.h>

namespace NPersonalCards {

const NInfra::TSensorGroup SENSOR_GROUP = NInfra::TSensorGroup("personal_cards");

class THistogramTimer {
public:
    THistogramTimer(
        const TStringBuf& name,
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {}
    );

    ~THistogramTimer();

private:
    const TInstant StartTime_;
    TIntrusivePtr<NInfra::THistogramRateSensor> Histogram_;

    static constexpr ui32 DEFAULT_BUCKETS_COUNT = 50;
    static constexpr NMonitoring::TBucketBound DEFAULT_START_VALUE = 0;
    static constexpr NMonitoring::TBucketBound DEFAULT_BUCKET_WIDTH = 10; // 10ms
};

} // namespace NPersonalCards
