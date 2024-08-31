#include "sensors.h"

namespace NPersonalCards {

THistogramTimer::THistogramTimer(
    const TStringBuf& name,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels
)
    : StartTime_(TInstant::Now())
{
    auto sensorGroup = SENSOR_GROUP;
    sensorGroup.AddLabels(labels);

    Histogram_ = MakeIntrusive<NInfra::THistogramRateSensor>(
        sensorGroup,
        name,
        NMonitoring::LinearHistogram(
            DEFAULT_BUCKETS_COUNT,
            DEFAULT_START_VALUE,
            DEFAULT_BUCKET_WIDTH
        )
    );
}

THistogramTimer::~THistogramTimer() {
    Histogram_->Add((TInstant::Now() - StartTime_).MilliSeconds());
}

} // namespace NPersonalCards
