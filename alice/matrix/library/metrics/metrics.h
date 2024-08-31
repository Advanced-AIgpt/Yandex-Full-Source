#pragma once

#include <infra/libs/sensors/sensor.h>
#include <infra/libs/sensors/sensor_group.h>

#include <util/datetime/base.h>


namespace NMatrix {

class TSourceMetrics : public TNonCopyable {
public:
    explicit TSourceMetrics(TStringBuf sourceName);
    ~TSourceMetrics();

    // Basic
    void PushAbs(
        i64 value,
        TStringBuf sensor,
        TStringBuf code = "",
        TStringBuf backend = "self",
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {}
    );
    void IncGauge(
        TStringBuf sensor,
        TStringBuf code = "",
        TStringBuf backend="self",
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {}
    );
    void DecGauge(
        TStringBuf sensor,
        TStringBuf code = "",
        TStringBuf backend="self",
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {}
    );

    void PushRate(
        i64 value,
        TStringBuf sensor,
        TStringBuf code = "",
        TStringBuf backend = "self",
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {}
    );
    void PushRate(
        TStringBuf sensor,
        TStringBuf code = "",
        TStringBuf backend = "self",
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {}
    );

    void InitHist(
        TStringBuf sensor,
        NMonitoring::IHistogramCollectorPtr&& histogramCollector,
        TStringBuf code = "",
        TStringBuf backend = "self",
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {}
    );
    void PushHist(
        i64 value,
        TStringBuf sensor,
        NMonitoring::IHistogramCollectorPtr&& histogramCollector,
        TStringBuf code = "",
        TStringBuf backend = "self",
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {}
    );

    void PushDurationHist(
        TDuration value,
        TStringBuf sensor,
        TStringBuf code = "",
        TStringBuf backend = "self",
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {},
        NMonitoring::IHistogramCollectorPtr&& histogramCollector = GetDefaultDurationHistogramCollector()
    );
    void PushTimeDiffWithNowHist(
        TInstant ref,
        TStringBuf sensor,
        TStringBuf code = "",
        TStringBuf backend = "self",
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {},
        NMonitoring::IHistogramCollectorPtr&& histogramCollector = GetDefaultDurationHistogramCollector()
    );

    // Status
    void SetStatus(TStringBuf code, bool error = false);
    void SetError(TStringBuf code);

    // Http
    void InitHttpCode(
        int httpCode,
        TStringBuf backend = "self",
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {}
    );
    void RateHttpCode(
        i64 value,
        int httpCode,
        TStringBuf backend = "self",
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {}
    );

    // Apphost
    void InitAppHostResponseOk(
        TStringBuf backend = "self",
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {}
    );
    void RateAppHostResponseOk(
        i64 value = 1,
        TStringBuf backend = "self",
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {}
    );
    void RateAppHostResponseError(
        i64 value = 1,
        TStringBuf backend = "self",
        const TVector<std::pair<TStringBuf, TStringBuf>>& labels = {}
    );

    static NMonitoring::IHistogramCollectorPtr GetDefaultDurationHistogramCollector();

private:
    NInfra::TSensorGroup SensorGroup_;

    TString Status_;
    bool Error_;
};

} // namespace NMatrix
