#include "engine.h"

namespace NVoice {
namespace NMetrics {

TMetricsEngine::TMetricsEngine()
{ }


TMetricsEngine& TMetricsEngine::SetBackend(EMetricsBackend backend, IBackendPtr ptr) {
    const int ret = static_cast<int>(backend);
    if (ret >= 0 && ret < static_cast<int>(EMetricsBackend::Max)) {
        // We are completely removing the previous backend
        // so we need to remove all cached sensor's refs
        Storages[ret].Get().Reset();

        Backends[ret] = std::move(ptr);
    }
    return *this;
}

TScopeMetrics TMetricsEngine::BeginScope(const TClientInfo& info, EMetricsBackend backend) {
    return TScopeMetrics { TIntrusivePtr<TMetricsEngine>(this), info, backend };
}


void TMetricsEngine::PushAbs(
    int64_t value,
    TStringBuf scope,
    TStringBuf sensor,
    TStringBuf code,
    TStringBuf backend,
    const TClientInfo& info,
    EMetricsBackend metricsBackend
) {
    TBackendData data = GetBackend(metricsBackend);
    if (!data) return;

    const TLabels labels = data.Backend->ApplyAggregationRules(info);

    const TString sensorName = data.Backend->BuildSensorName(scope, sensor, code, backend, labels);

    ISensorPtr sens = data.Storage->GetSensor(sensorName, code, labels);
    if (!sens) {
        ISensorPtr sensor = data.Backend->CreateAbsoluteSensor(sensorName, code, labels);
        if (!sensor) {
            return;
        }
        sens = data.Storage->AddSensor(sensorName, code, labels, std::move(sensor));
    }

    sens->Push(value);
}


void TMetricsEngine::PushRate(
    int64_t value,
    TStringBuf scope,
    TStringBuf sensor,
    TStringBuf code,
    TStringBuf backend,
    const TClientInfo& info,
    EMetricsBackend metricsBackend
) {
    TBackendData data = GetBackend(metricsBackend);
    if (!data) return;

    const TLabels labels = data.Backend->ApplyAggregationRules(info);

    const TString sensorName = data.Backend->BuildSensorName(scope, sensor, code, backend, labels);

    ISensorPtr sens = data.Storage->GetSensor(sensorName, code, labels);
    if (!sens) {
        ISensorPtr sensor = data.Backend->CreateRateSensor(sensorName, code, labels);
        if (!sensor) {
            return;
        }
        sens = data.Storage->AddSensor(sensorName, code, labels, std::move(sensor));
    }

    sens->Push(value);
}


void TMetricsEngine::PushHist(
    int64_t value,
    TStringBuf scope,
    TStringBuf sensor,
    TStringBuf code,
    TStringBuf backend,
    const TClientInfo& info,
    EMetricsBackend metricsBackend
) {
    TBackendData data = GetBackend(metricsBackend);
    if (!data) return;

    const TLabels labels = data.Backend->ApplyAggregationRules(info);

    const TString sensorName = data.Backend->BuildSensorName(scope, sensor, code, backend, labels);

    ISensorPtr sens = data.Storage->GetSensor(sensorName, code, labels);
    if (!sens) {
        ISensorPtr sensor = data.Backend->CreateHistogramSensor(sensorName, code, labels);
        if (!sensor) {
            return;
        }
        sens = data.Storage->AddSensor(sensorName, code, labels, std::move(sensor));
    }

    sens->Push(value);
}


TMetricsEngine::TBackendData TMetricsEngine::GetBackend(EMetricsBackend metricsBackend) {
    const int ret = static_cast<int>(metricsBackend);
    if (ret < 0 || ret >= static_cast<int>(EMetricsBackend::Max)) {
        return { nullptr, nullptr };
    }

    return {
        Backends[ret].Get(),
        Storages[ret].GetPtr(),
    };
}


bool TMetricsEngine::SerializeMetrics(EMetricsBackend backend, IOutputStream& stream, EOutputFormat format) {
    TBackendData data = GetBackend(backend);
    if (!data) {
        return false;
    }
    return data.Backend->SerializeMetrics(stream, format);
}


}   // namespace NMetrics
}   // namespace NVoice
