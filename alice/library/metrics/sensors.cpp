#include "sensors.h"

namespace NAlice::NMetrics {

// ISensors
void ISensors::AddRate(const NMonitoring::TLabels& labels, i32 delta) {
    NMonitoring::TLabels labelsCopy{labels};
    AddRate(std::move(labelsCopy), delta);
}

void ISensors::IncRate(const NMonitoring::TLabels& labels) {
    NMonitoring::TLabels labelsCopy{labels};
    IncRate(std::move(labelsCopy));
}

void ISensors::AddHistogram(const NMonitoring::TLabels& labels, ui64 value, const TVector<double>& bins) {
    NMonitoring::TLabels labelsCopy{labels};
    AddHistogram(std::move(labelsCopy), value, bins);
}

void ISensors::AddIntGauge(const NMonitoring::TLabels& labels, i64 value) {
    NMonitoring::TLabels labelsCopy{labels};
    AddIntGauge(std::move(labelsCopy), value);
}

void ISensors::SetIntGauge(const NMonitoring::TLabels& labels, i64 value) {
    NMonitoring::TLabels labelsCopy{labels};
    SetIntGauge(std::move(labelsCopy), value);
}


// TSensorsWrapper
TSensorsWrapper::TSensorsWrapper(const NMonitoring::TLabels& labels, ISensors& baseSensors)
    : BaseSensors_{baseSensors}
{
    for (const auto& label : labels) {
        CommonLabels_.emplace_back(TString{label.Name()}, TString{label.Value()});
    }
}

void TSensorsWrapper::AddRate(NMonitoring::TLabels&& labels, i32 delta) {
    EnrichLabels(labels);
    BaseSensors_.AddRate(std::move(labels), delta);
}

void TSensorsWrapper::IncRate(NMonitoring::TLabels&& labels) {
    EnrichLabels(labels);
    BaseSensors_.IncRate(std::move(labels));
}

void TSensorsWrapper::AddHistogram(NMonitoring::TLabels&& labels, ui64 value, const TVector<double>& bins) {
    EnrichLabels(labels);
    BaseSensors_.AddHistogram(std::move(labels), value, bins);
}

void TSensorsWrapper::AddIntGauge(NMonitoring::TLabels&& labels, i64 value) {
    EnrichLabels(labels);
    BaseSensors_.AddIntGauge(std::move(labels), value);
}

void TSensorsWrapper::SetIntGauge(NMonitoring::TLabels&& labels, i64 value) {
    EnrichLabels(labels);
    BaseSensors_.SetIntGauge(std::move(labels), value);
}

void TSensorsWrapper::EnrichLabels(NMonitoring::TLabels& labels) {
    for (const auto& [key, value] : CommonLabels_) {
        labels.Add(key, value);
    }
}

} // namespace NAlice::NMetrics