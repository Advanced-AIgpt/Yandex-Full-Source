#pragma once

#include "engine.h"

#include <util/generic/singleton.h>


namespace NVoice {
namespace NMetrics {


class TMetrics {
public:
    Y_DECLARE_SINGLETON_FRIEND()

    static inline TMetrics& Instance() {
        return *Singleton<TMetrics>();
    }

    inline TScopeMetrics BeginScope(const TClientInfo& info, EMetricsBackend backend) {
        return Engine->BeginScope(info, backend);
    }

    inline TMetrics& SetBackend(EMetricsBackend backend, IBackendPtr ptr) {
        Engine->SetBackend(backend, std::move(ptr));
        return *this;
    }

    inline void Reset() {
        Engine->Reset();
    }

    inline bool SerializeMetrics(EMetricsBackend backend, IOutputStream& stream, EOutputFormat format) {
        return Engine->SerializeMetrics(backend, stream, format);
    }

    void PushAbs(int64_t value, TStringBuf scope, TStringBuf sensor, TStringBuf code, TStringBuf backend, const TClientInfo& info, EMetricsBackend metricsBackend) {
        Engine->PushAbs(value, scope, sensor, code, backend, info, metricsBackend);
    }

    void PushRate(int64_t value, TStringBuf scope, TStringBuf sensor, TStringBuf code, TStringBuf backend, const TClientInfo& info, EMetricsBackend metricsBackend) {
        Engine->PushRate(value, scope, sensor, code, backend, info, metricsBackend);
    }

    void PushHist(int64_t value, TStringBuf scope, TStringBuf sensor, TStringBuf code, TStringBuf backend, const TClientInfo& info, EMetricsBackend metricsBackend) {
        Engine->PushHist(value, scope, sensor, code, backend, info, metricsBackend);
    }

private:
    TMetrics()
        : Engine(MakeIntrusive<TMetricsEngine>())
    { }

private:
    TIntrusivePtr<TMetricsEngine> Engine;
};

class TSourceMetrics : public TScopeMetrics {
public:
    TSourceMetrics(
        TIntrusivePtr<TMetricsEngine> engine,
        TStringBuf sourceName,
        const TClientInfo& clientInfo,
        EMetricsBackend metricsBackend
    )
        : TScopeMetrics(
            engine->BeginScope(
                clientInfo,
                metricsBackend
            )
        )
    {
        OnScopeStarted(sourceName);
    }

    TSourceMetrics(
        TMetrics& metrics,
        TStringBuf sourceName,
        const TClientInfo& clientInfo,
        EMetricsBackend metricsBackend
    )
        : TScopeMetrics(
            metrics.BeginScope(
                clientInfo,
                metricsBackend
            )
        )
    {
        OnScopeStarted(sourceName);
    }

    // Engine autodetection
    TSourceMetrics(
        TStringBuf sourceName,
        const TClientInfo& clientInfo,
        EMetricsBackend metricsBackend
    )
        : TScopeMetrics(
            TMetrics::Instance().BeginScope(
                clientInfo,
                metricsBackend
            )
        )
    {
        OnScopeStarted(sourceName);
    }

    ~TSourceMetrics() {
        TStringBuf code;

        if (Error_) {
            code = Status_.empty() ? TStringBuf("error") : TStringBuf(Status_);
        } else {
            code = Status_.empty() ? TStringBuf("ok") : TStringBuf(Status_);
        }

        OnScopeCompleted(code);
    }

    inline void SetStatus(TStringBuf code, bool error = false) {
        Status_ = code;
        Error_ = error;
    }

    inline void SetError(TStringBuf code) {
        SetStatus(code, true);
    }

    void RateHttpCode(int httpCode, TStringBuf backend = "self", int64_t value = 1);

    static TClientInfo MakeEmptyClientInfo();

private:
    TString Status_ = "ok";
    bool Error_ = false;
};

NMonitoring::TBucketBounds MakeMillisBuckets();

NMonitoring::TBucketBounds MakeMicrosBuckets();

}   // namespace NMetrics
}   // namespace NVoice
