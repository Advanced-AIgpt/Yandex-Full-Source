#include "service.h"
#include "unistat.h"
#include "util.h"

#include <library/cpp/json/writer/json.h>
#include <library/cpp/monlib/metrics/metric_type.h>
#include <library/cpp/monlib/metrics/labels.h>
#include <library/cpp/monlib/metrics/metric_registry.h>

#include <util/generic/maybe.h>
#include <util/string/builder.h>

namespace NAlice::NMetrics {
namespace {

class TGolovanSensorsConsumer : public NMonitoring::IMetricConsumer {
public:
    explicit TGolovanSensorsConsumer(NJsonWriter::TBuf& json)
        : Json(json)
    {
    }

    void OnStreamBegin() override {
        Json.BeginList();
    }

    void OnStreamEnd() override {
        Json.EndList();
    }

    void OnCommonTime(TInstant /*time*/) override {
    }

    void OnMetricBegin(NMonitoring::EMetricType kind) override {
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
                value.StartsWith("source_");
        } else {
            Signal << '_';
            IsFirstLabel = false;
        }
        if (name != TStringBuf("sensor")) { // sensor is useless but consumes expensive chars limit at yasm server side
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

    void OnHistogram(TInstant /*time*/, NMonitoring::IHistogramSnapshotPtr /*snapshot*/) override {
    }

    void OnLogHistogram(TInstant /*time*/, NMonitoring::TLogHistogramSnapshotPtr /*snapshot*/) override {
    }

    void OnSummaryDouble(TInstant /*time*/, NMonitoring::ISummaryDoubleSnapshotPtr /*snapshot*/) override {
    }


private:
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

TGolovanCountersPage::TGolovanCountersPage(const TString& path, NMonitoring::TMetricRegistry& sensors)
    : IMonPage(path)
    , Sensors(sensors)
{
}

TString TGolovanCountersPage::DumpSensors() {
    NJsonWriter::TBuf buf;
    TGolovanSensorsConsumer consumer(buf);

    Sensors.Accept(TInstant::Now(), &consumer);

    return buf.Str();
}

void TGolovanCountersPage::Output(NMonitoring::IMonHttpRequest& request) {
    request.Output() << NMonitoring::HTTPOKJSON << DumpSensors();
}

} // namespace NAlice::NMetrics
