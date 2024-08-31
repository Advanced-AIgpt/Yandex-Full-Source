#include "metrics.h"

#include <library/cpp/http/misc/httpcodes.h>

#include <util/digest/multi.h>
#include <util/string/cast.h>
#include <util/system/env.h>
#include <util/system/rwlock.h>

namespace NMatrix {

namespace {

static const NInfra::TSensorGroup DEFAULT_SENSOR_GROUP = []() -> NInfra::TSensorGroup {
    NInfra::TSensorGroup sensorGroup("");

    return sensorGroup;
}();

static constexpr int MIN_HTTP_CODE = 100;
static constexpr int MAX_HTTP_CODE = 600;
static const TVector<TString> HTTP_REASON_NAMES = []() {
    TVector<TString> httpReasonNames;

    static auto convertToLabel = [](const TStringBuf str) {
        TString ret;
        ret.reserve(str.size());
        for (const char c : str) {
            if (c == ' ') {
                ret.push_back('_');
            } else {
                ret.push_back(AsciiToLower(c));
            }
        }

        return ret;
    };

    for (int i = MIN_HTTP_CODE; i < MAX_HTTP_CODE; ++i) {
        if (IsHttpCode(i)) {
            httpReasonNames.push_back(convertToLabel(HttpCodeStrEx(i)));
        } else {
            httpReasonNames.push_back(ToString(i));
        }
    }

    return httpReasonNames;
}();

} // namespace

TSourceMetrics::TSourceMetrics(TStringBuf sourceName)
    : SensorGroup_(sourceName)
    , Status_("ok")
    , Error_(false)
{
   PushAbs(1, "inprogress", "");
   PushRate(1, "in", "");
}

TSourceMetrics::~TSourceMetrics() {
    TStringBuf code;

    if (Error_) {
        code = Status_.empty() ? TStringBuf("error") : TStringBuf(Status_);
    } else {
        code = Status_.empty() ? TStringBuf("ok") : TStringBuf(Status_);
    }

    PushAbs(-1, "inprogress", "");
    PushRate(1, "completed", code);
}

void TSourceMetrics::PushAbs(
    i64 value,
    TStringBuf sensor,
    TStringBuf code,
    TStringBuf backend,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels
) {
    NInfra::TSensorGroup sensorGroup = NInfra::TSensorGroup(SensorGroup_, backend);
    if (!code.empty()) {
        sensorGroup.AddLabel("code", code);
    }
    if (!labels.empty()) {
        sensorGroup.AddLabels(labels);
    }
    NInfra::TIntGaugeSensor(sensorGroup, sensor).Add(value);
}
void TSourceMetrics::IncGauge(
    TStringBuf sensor,
    TStringBuf code,
    TStringBuf backend,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels
) {
    return PushAbs(1, sensor, code, backend, labels);
}
void TSourceMetrics::DecGauge(
    TStringBuf sensor,
    TStringBuf code,
    TStringBuf backend,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels
) {
    return PushAbs(-1, sensor, code, backend, labels);
}

void TSourceMetrics::PushRate(
    i64 value,
    TStringBuf sensor,
    TStringBuf code,
    TStringBuf backend,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels
) {
    NInfra::TSensorGroup sensorGroup = NInfra::TSensorGroup(SensorGroup_, backend);
    if (!code.empty()) {
        sensorGroup.AddLabel("code", code);
    }
    if (!labels.empty()) {
        sensorGroup.AddLabels(labels);
    }
    NInfra::TRateSensor(sensorGroup, sensor).Add(value);
}
void TSourceMetrics::PushRate(
    TStringBuf sensor,
    TStringBuf code,
    TStringBuf backend,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels
) {
    return PushRate(1, sensor, code, backend, labels);
}

void TSourceMetrics::InitHist(
    TStringBuf sensor,
    NMonitoring::IHistogramCollectorPtr&& histogramCollector,
    TStringBuf code,
    TStringBuf backend,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels
) {
    NInfra::TSensorGroup sensorGroup = NInfra::TSensorGroup(SensorGroup_, backend);
    if (!code.empty()) {
        sensorGroup.AddLabel("code", code);
    }
    if (!labels.empty()) {
        sensorGroup.AddLabels(labels);
    }
    NInfra::THistogramRateSensor(sensorGroup, sensor, std::move(histogramCollector));
}
void TSourceMetrics::PushHist(
    i64 value,
    TStringBuf sensor,
    NMonitoring::IHistogramCollectorPtr&& histogramCollector,
    TStringBuf code,
    TStringBuf backend,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels
) {
    NInfra::TSensorGroup sensorGroup = NInfra::TSensorGroup(SensorGroup_, backend);
    if (!code.empty()) {
        sensorGroup.AddLabel("code", code);
    }
    if (!labels.empty()) {
        sensorGroup.AddLabels(labels);
    }
    NInfra::THistogramRateSensor(sensorGroup, sensor, std::move(histogramCollector)).Add(value);
}
void TSourceMetrics::PushDurationHist(
    TDuration value,
    TStringBuf sensor,
    TStringBuf code,
    TStringBuf backend,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels,
    NMonitoring::IHistogramCollectorPtr&& histogramCollector
) {
    PushHist(value.MicroSeconds(), sensor, std::move(histogramCollector), code, backend, labels);
}
void TSourceMetrics::PushTimeDiffWithNowHist(
    TInstant ref,
    TStringBuf sensor,
    TStringBuf code,
    TStringBuf backend,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels,
    NMonitoring::IHistogramCollectorPtr&& histogramCollector
) {
    PushDurationHist(TInstant::Now() - ref, sensor, code, backend, labels, std::move(histogramCollector));
}

void TSourceMetrics::SetStatus(TStringBuf code, bool error) {
    Status_ = code;
    Error_ = error;
}

void TSourceMetrics::SetError(TStringBuf code) {
    SetStatus(code, true);
}

void TSourceMetrics::InitHttpCode(
    int httpCode,
    TStringBuf backend,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels
) {
    RateHttpCode(0, httpCode, backend, labels);
}
void TSourceMetrics::RateHttpCode(
    i64 value,
    int httpCode,
    TStringBuf backend,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels
) {
    static constexpr TStringBuf HTTP_REASON_UNKNOWN = "unknown";

    if (MIN_HTTP_CODE <= httpCode && httpCode < MAX_HTTP_CODE) {
        PushRate(value, "response", HTTP_REASON_NAMES[httpCode - MIN_HTTP_CODE], backend, labels);
    } else {
        PushRate(value, "response", HTTP_REASON_UNKNOWN, backend, labels);
    }
}

void TSourceMetrics::InitAppHostResponseOk(
    TStringBuf backend,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels
) {
    RateAppHostResponseOk(0, backend, labels);
}
void TSourceMetrics::RateAppHostResponseOk(
    i64 value,
    TStringBuf backend,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels
) {
    PushRate(value, "response", "ok", backend, labels);
}
void TSourceMetrics::RateAppHostResponseError(
    i64 value,
    TStringBuf backend,
    const TVector<std::pair<TStringBuf, TStringBuf>>& labels
) {
    PushRate(value, "response", "error", backend, labels);
}

NMonitoring::IHistogramCollectorPtr TSourceMetrics::GetDefaultDurationHistogramCollector() {
    // For backward compatibility with voice sensors
    static NMonitoring::TBucketBounds buckets = {
        TDuration::Zero().MicroSeconds(),

        TDuration::MilliSeconds(5).MicroSeconds(),
        TDuration::MilliSeconds(10).MicroSeconds(),
        TDuration::MilliSeconds(25).MicroSeconds(),
        TDuration::MilliSeconds(50).MicroSeconds(),
        TDuration::MilliSeconds(75).MicroSeconds(),
        TDuration::MilliSeconds(100).MicroSeconds(),
        TDuration::MilliSeconds(125).MicroSeconds(),
        TDuration::MilliSeconds(150).MicroSeconds(),
        TDuration::MilliSeconds(175).MicroSeconds(),
        TDuration::MilliSeconds(200).MicroSeconds(),
        TDuration::MilliSeconds(250).MicroSeconds(),
        TDuration::MilliSeconds(500).MicroSeconds(),
        TDuration::MilliSeconds(750).MicroSeconds(),

        TDuration::Seconds(1).MicroSeconds(),
        TDuration::MilliSeconds(1250).MicroSeconds(),
        TDuration::MilliSeconds(1500).MicroSeconds(),
        TDuration::Seconds(2).MicroSeconds(),
        TDuration::MilliSeconds(2500).MicroSeconds(),
        TDuration::Seconds(3).MicroSeconds(),
        TDuration::MilliSeconds(3500).MicroSeconds(),
        TDuration::Seconds(4).MicroSeconds(),
        TDuration::MilliSeconds(4500).MicroSeconds(),
        TDuration::Seconds(5).MicroSeconds(),
        TDuration::Seconds(6).MicroSeconds(),
        TDuration::MilliSeconds(6500).MicroSeconds(),
        TDuration::Seconds(7).MicroSeconds(),
        TDuration::MilliSeconds(7500).MicroSeconds(),
        TDuration::Seconds(8).MicroSeconds(),
        TDuration::Seconds(9).MicroSeconds(),
        TDuration::Seconds(10).MicroSeconds(),
        TDuration::MilliSeconds(17500).MicroSeconds(),
    };

    return NMonitoring::ExplicitHistogram(buckets);
}

} // namespace NMatrix
