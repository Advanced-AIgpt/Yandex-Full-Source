#include "metrics.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/json/writer/json.h>
#include <library/cpp/monlib/metrics/metric_type.h>

#include <util/generic/maybe.h>
#include <util/generic/singleton.h>
#include <util/string/builder.h>

namespace NMonitoring {
namespace {

const NUnistat::TIntervals TIME_INTERVALS =
    { 1, 5, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 250, 300, 350, 400, 450, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 3000, 4000, 5000, 6000, 10000 };

class TGolovanSensorsConsumer : public IMetricConsumer {
public:
    TGolovanSensorsConsumer (NJsonWriter::TBuf& json)
        : Json(json)
    {
    }

    void OnStreamBegin() override {
        Json.BeginList();
        AddBassCounters();
    }

    void OnStreamEnd() override {
        Json.EndList();
    }

    void OnCommonTime(TInstant /*time*/) override {
    }

    void OnMetricBegin(EMetricType kind) override {
        MetricType = kind;
    }

    void OnMetricEnd() override {
        MetricType.Clear();
    }

    void OnLabelsBegin() override {
        Signal.Str().clear();
        IsValidForYasm = false;
        IsFirstLabel = true;
    }

    void OnLabelsEnd() override {
        if (!MetricType.Defined()) {
            return;
        }
        switch (MetricType.GetRef()) {
            case NMonitoring::EMetricType::GAUGE:
            case NMonitoring::EMetricType::IGAUGE:
            case NMonitoring::EMetricType::COUNTER:
                Signal << "_ammm";
                break;
            case NMonitoring::EMetricType::RATE:
                Signal << "_dmmm";
                break;
            case NMonitoring::EMetricType::UNKNOWN:
            case NMonitoring::EMetricType::HIST:
            case NMonitoring::EMetricType::LOGHIST:
            case NMonitoring::EMetricType::HIST_RATE:
            case NMonitoring::EMetricType::DSUMMARY:
                // not supported yet: different formats
                IsValidForYasm = false;
                break;
        }
    }

    void OnLabel(const TStringBuf name, const TStringBuf value) override {
        if (IsFirstLabel) {
            IsValidForYasm =
                value.StartsWith("http_") ||
                value.StartsWith("source_") ||
                value.StartsWith("extskill_") ||
                value.StartsWith("bass_computer_vision_");
        } else {
            Signal << '_';
            IsFirstLabel = false;
        }
        if (name != "sensor") { // sensor is useless but consumes expensive chars limit at yasm server side
            Signal << name << '_';
        }
        Signal << value;
    }

    void OnDouble(TInstant /*time*/, double value) override {
        OnValue(value);
    }

    void OnInt64(TInstant /*time*/, i64 value) override {
        OnValue(value);
    }

    void OnUint64(TInstant /*time*/, ui64 value) override {
        OnValue(value);
    }

    void OnHistogram(TInstant /*time*/, IHistogramSnapshotPtr /*snapshot*/) override {
    }

    void OnLogHistogram(TInstant /*time*/, TLogHistogramSnapshotPtr /*snapshot*/) override {
    }

    void OnSummaryDouble(TInstant /*time*/, ISummaryDoubleSnapshotPtr /*snapshot*/) override {
    }

private:
    void AddBassCounters() {
        TBassCounters* bassCounters = TBassCounters::Counters();
        if (!bassCounters) {
            LOG(ERR) << "Using bass couters before initialization" << Endl;
            return;
        }

        if (AtomicGet(bassCounters->Initialized)) {
            for (auto& name2hgram : bassCounters->UnistatHistograms) {
                name2hgram.second->PrintValue(Json);
            }
        }
    }

    void OnValue(double value) {
        if (!IsValidForYasm) {
            return;
        }
        Json.BeginList()
                .WriteString(Signal.Str())
                .WriteDouble(value)
            .EndList();
    }

private:
    NJsonWriter::TBuf& Json;
    TString Prefix;

    bool IsValidForYasm = false;
    bool IsFirstLabel = true;
    TStringStream Signal;
    TMaybe<NMonitoring::EMetricType> MetricType;
};

} // namespace

TIntrusivePtr<TBassCounters> TBassCounters::BassCounters_;

TMetricRegistry& GetSensors() {
    if (TMetricRegistry* sensors = Singleton<TMetricRegistryPtr>()->Get()) {
        return *sensors;
    }
    static TMetricRegistry sensors;
    return sensors;
}

NMonitoring::THistogram& GetHistogram(const TString& name) {

    // sensor_0 = 5 ms
    // sensor_i = 5 ms * (1.4 ^ i)
    // sensor_24 = 16070 ms
    constexpr ui32 numBuckets = 25;
    constexpr double base = 1.4;
    constexpr double scale = 5;

    return *GetSensors().HistogramRate({{"histogram", NMonitoring::NormalizeMetricNameForGolovan(name)}}, NMonitoring::ExponentialHistogram(numBuckets, base, scale));
}

void SetSensorsSingleton(TMetricRegistryPtr sensors) {
    *(Singleton<TMetricRegistryPtr>()) = sensors;
}

// static
TBassCounters* TBassCounters::Counters() {
    return BassCounters_.Get();
}

// static
void TBassCounters::SetCountersSingleton(TIntrusivePtr<TBassCounters> counters) {
    if (!BassCounters_) {
        BassCounters_ = std::move(counters);
    }
}

void TBassCounters::RegisterUnistatHistogram(TStringBuf name) {
    UnistatHistograms.emplace(name, nullptr);
}

void TBassCounters::InitUnistat() {
    for (auto& pair : UnistatHistograms) {
        const TString& name = pair.first;
        pair.second = TUnistat::Instance().DrillHistogramHole(name, "hgram", NUnistat::TPriority(0), TIME_INTERVALS);
    }
    AtomicSet(Initialized, 1);
}

TBassGolovanCountersPage::TBassGolovanCountersPage(const TString& path, TMetricRegistryPtr sensors)
    : IMonPage(path)
    , Sensors(sensors)
{
}

void TBassGolovanCountersPage::Output(NMonitoring::IMonHttpRequest& request) {
    NJsonWriter::TBuf buf;
    TGolovanSensorsConsumer consumer(buf);

    Sensors->Accept(TInstant::Now(), &consumer);

    request.Output() << HTTPOKJSON;
    buf.FlushTo(&request.Output());
}

/**
 * Golovan does not accept repeated '__', ony one '_' at a time is allowed
 */
TString NormalizeMetricNameForGolovan(TStringBuf name) {
    TStringBuilder b;
    b.reserve(name.size());

    while (name && name[0] == '_') {
        name.Skip(1);
    }
    while (name && name.back() == '_') {
        name.Chop(1);
    }

    while (name) {
        TStringBuf token = name.NextTok('_');
        b << token;
        if (name.empty()) {
            break;
        }

        while (name && name[0] == '_') {
            name.Skip(1);
        }
        if (name) {
            b << '_';
        }
    }
    return b;
}

namespace {

NMonitoring::TIntGauge* GetSensor(TMetricRegistry& sensors, NMonitoring::TLabels labels, const TString& name) {
    labels.Add(TStringBuf("sensor"), name);
    return sensors.IntGauge(labels);
}

} // namespace

void TCountersChanger::Add(NMonitoring::TLabels labels, const TString& name, int delta) {
    GetSensor(Sensors, labels, name)->Add(delta);
}

void TCountersChanger::Inc(NMonitoring::TLabels labels, const TString& name) {
    GetSensor(Sensors, labels, name)->Inc();
}

} // namespace NMonitoring
