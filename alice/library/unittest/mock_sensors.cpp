#include "mock_sensors.h"

namespace NAlice {

// TFakeSensors ---------------------------------------------------------------
const TFakeSensors::TRateSensor* TFakeSensors::FindFirstRateSensor(TStringBuf name, TStringBuf value) const {
    for (const auto& [key, sensor] : RateCounters_) {
        if (const auto label = sensor.Labels.Get(name); label.has_value() && (*label)->Value() == value) {
            return &sensor;
        }
    }
    return nullptr;
}

TFakeSensors::TRateSensor& TFakeSensors::GetRateSensor(const NMonitoring::TLabels& labels) {
    auto* counter = RateCounters_.FindPtr(labels.Hash());
    if (!counter) {
        counter = &RateCounters_.emplace(labels.Hash(), TRateSensor{labels, 0}).first->second;
    }
    return *counter;
}

} // namespace NAlice
