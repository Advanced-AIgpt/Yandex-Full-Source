#pragma once

#include "golovan.h"
#include "solomon.h"
#include "storage.h"

#include <util/datetime/base.h>
#include <util/system/tls.h>


namespace NVoice {
namespace NMetrics {


class TScopeMetrics;


class TMetricsEngine : public TThrRefBase {
public:
    TMetricsEngine();

    TMetricsEngine& SetBackend(EMetricsBackend backend, IBackendPtr ptr);

    inline void Reset() {
        for (auto& ptr : Backends) {
            if (ptr) {
                // We are reset sensor's values but we are not removing them
                // So there is no need to reset storage
                ptr->Reset();
            }
        }
    }

public:     /* ScopeMetrics API */
    /**
     *  @brief Creates a request object
     */
    TScopeMetrics BeginScope(const TClientInfo& info, EMetricsBackend backend);

    /**
     *  @brief Solomon's IGAUGE or Golovan's _ammx (_max)
     *  e.g. for inprogress sessions
     */
    void PushAbs(int64_t value, TStringBuf scope, TStringBuf sensor, TStringBuf code, TStringBuf backend, const TClientInfo& info, EMetricsBackend metricsBackend);

    /**
     *  @brief Solomon's RATE or Golovan's _dmmm (_summ)
     *  e.g. for RPS
     */
    void PushRate(int64_t value, TStringBuf scope, TStringBuf sensor, TStringBuf code, TStringBuf backend, const TClientInfo& info, EMetricsBackend metricsBackend);

    /**
     *  @brief Solomon's HIST_RATE or Golovan's _dhhh (_hgram)
     *  e.g. for response time or response size
     */
    void PushHist(int64_t value, TStringBuf scope, TStringBuf sensor, TStringBuf code, TStringBuf backend, const TClientInfo& info, EMetricsBackend metricsBackend);

    /**
     *  @brief Serializes metrics, uses SPACK format for Solomon and JSON for Golovan
     */
    bool SerializeMetrics(EMetricsBackend backend, IOutputStream& stream, EOutputFormat format);

private:
    struct TBackendData {
        IBackend* Backend { nullptr };
        TSensorStorage* Storage { nullptr };

        inline operator bool() const {
            return Backend != nullptr && Storage != nullptr;
        }
    };

    TBackendData GetBackend(EMetricsBackend metricsBackend);

private:
    static constexpr int BackendsTypeCount = static_cast<int>(EMetricsBackend::Max);

    IBackendPtr     Backends[BackendsTypeCount];

    Y_THREAD(TSensorStorage) Storages[BackendsTypeCount];
};


class TScopeMetrics {
public:
    TScopeMetrics(TIntrusivePtr<TMetricsEngine> engine, TClientInfo info, EMetricsBackend metricsBackend)
        : Engine(engine)
        , ClientInfo(std::move(info))
        , MetricsBackend(metricsBackend)
    { }

    TScopeMetrics(TScopeMetrics&&) = default;

    virtual ~TScopeMetrics() = default;

    void OnScopeStarted(TStringBuf scopeName) {
        ScopeName = scopeName;
        PushAbs(1, "inprogress", "");
        PushRate(1, "in", "");
    }

    void OnScopeCompleted(TStringBuf code = "") {
        PushAbs(-1, "inprogress", "");
        PushRate(1, "completed", code);
    }

    void OnPartialResult(int64_t value, TStringBuf sensor, TStringBuf backend = "self", TStringBuf code = "") {
        PushRate(value, sensor, code, backend);
    }

    void OnPartialResult(TStringBuf sensor, TStringBuf backend = "self", TStringBuf code = "") {
        OnPartialResult(1, sensor, backend, code);
    }

    void OnPartialResult(TDuration value, TStringBuf sensor, TStringBuf backend = "self", TStringBuf code = "") {
        PushHist(value.MicroSeconds(), sensor, code, backend);
    }

    void OnPartialResult(TInstant ref, TInstant value, TStringBuf sensor, TStringBuf backend = "self", TStringBuf code = "") {
        OnPartialResult(value - ref, sensor, backend, code);
    }

public:     /* direct metrics api */
    TScopeMetrics& PushAbs(int64_t value, TStringBuf sensor, TStringBuf code = "", TStringBuf backend = "self") {
        Engine->PushAbs(value, ScopeName, sensor, code, backend, ClientInfo, MetricsBackend);
        return *this;
    }

    TScopeMetrics& IncGauge(TStringBuf sensor, TStringBuf code = "", TStringBuf backend="self") {
        return PushAbs(1, sensor, code, backend);
    }

    TScopeMetrics& DecGauge(TStringBuf sensor, TStringBuf code = "", TStringBuf backend="self") {
        return PushAbs(-1, sensor, code, backend);
    }

    TScopeMetrics& PushRate(int64_t value, TStringBuf sensor, TStringBuf code = "", TStringBuf backend = "self") {
        Engine->PushRate(value, ScopeName, sensor, code, backend, ClientInfo, MetricsBackend);
        return *this;
    }

    TScopeMetrics& PushRate(TStringBuf sensor, TStringBuf code = "", TStringBuf backend = "self") {
        return PushRate(1, sensor, code, backend);
    }

    TScopeMetrics& PushHist(int64_t value, TStringBuf sensor, TStringBuf code = "", TStringBuf backend = "self") {
        Engine->PushHist(value, ScopeName, sensor, code, backend, ClientInfo, MetricsBackend);
        return *this;
    }

    TScopeMetrics& PushHist(TDuration value, TStringBuf sensor, TStringBuf code = "", TStringBuf backend = "self") {
        return PushHist(value.MicroSeconds(), sensor, code, backend);
    }

    TScopeMetrics& PushHist(TInstant ref, TStringBuf sensor, TStringBuf code = "", TStringBuf backend = "self") {
        return PushHist(TInstant::Now() - ref, sensor, code, backend);
    }

    TScopeMetrics& PushHist(TInstant ref, TInstant value, TStringBuf sensor, TStringBuf code = "", TStringBuf backend = "self") {
        return PushHist(value - ref, sensor, code, backend);
    }

protected:
    TIntrusivePtr<TMetricsEngine> Engine;
    TString         ScopeName;
    TClientInfo     ClientInfo;
    EMetricsBackend MetricsBackend;
};


}   // namespace NMetrics
}   // namespace NVoice
