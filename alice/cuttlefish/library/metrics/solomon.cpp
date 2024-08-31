#include "solomon.h"

#include <library/cpp/monlib/encode/json/json.h>
#include <library/cpp/monlib/encode/spack/spack_v1.h>
#include <util/system/env.h>


namespace NVoice {
namespace NMetrics {


class TSolomonSensor : public ISensor {
public:
    TSolomonSensor(NMonitoring::IIntGauge* ptr)
        : Type(ESensorType::Gauge)
        , GaugePtr(ptr)
    { }

    TSolomonSensor(NMonitoring::IRate* ptr)
        : Type(ESensorType::Rate)
        , RatePtr(ptr)
    { }

    TSolomonSensor(NMonitoring::IHistogram* ptr)
        : Type(ESensorType::Hist)
        , HistPtr(ptr)
    { }

    void Push(int64_t value) override {
        switch (Type) {
            case ESensorType::Gauge:
                GaugePtr->Add(value);
                return;
            case ESensorType::Rate:
                RatePtr->Add(value);
                return;
            case ESensorType::Hist:
                HistPtr->Record(value);
        }
    }

private:
    ESensorType Type;
    union {
        NMonitoring::IIntGauge*     GaugePtr;
        NMonitoring::IRate*         RatePtr;
        NMonitoring::IHistogram*    HistPtr;
    };
};


TSolomonBackend::TSolomonBackend(TAggregationRules rules, const NMonitoring::TBucketBounds& buckets, TStringBuf service, bool maskHost)
    : IBackend(std::move(rules))
    , Buckets_(buckets)
    , Registry_({{"service", TString(service)}})
    , MaskHostname_(maskHost)
    , CType_(GetHostCType())
{
    std::sort(Buckets_.begin(), Buckets_.end());
}


TSolomonBackend::~TSolomonBackend()
{ }


void TSolomonBackend::FillLabels(NMonitoring::TLabels& solomonLabels, const TString& sensorName, TStringBuf code, const TLabels& labels) const {
    solomonLabels.Add("sensor", sensorName);
    solomonLabels.Add("device", labels.DeviceName);
    solomonLabels.Add("code", code);
    solomonLabels.Add("surface", labels.GroupName);
    solomonLabels.Add("surface_type", labels.SubgroupName);
    solomonLabels.Add("app_id", labels.AppId);
    if (labels.ClientType != EClientType::None) {
        solomonLabels.Add("client_type", labels.ClientType == EClientType::User ? "user" : "robot");
    }
    solomonLabels.Add("ctype", CType_);
    if (MaskHostname_) {
        solomonLabels.Add("host", "ANY");
    }
}


ISensorPtr TSolomonBackend::CreateAbsoluteSensor(const TString& sensorName, TStringBuf code, TLabels labels) {
    NMonitoring::TLabels lbls;
    FillLabels(lbls, sensorName, code, labels);
    return RegisterSensor(new TSolomonSensor(Registry_.IntGauge(lbls)));
}


ISensorPtr TSolomonBackend::CreateRateSensor(const TString& sensorName, TStringBuf code, TLabels labels) {
    NMonitoring::TLabels lbls;
    FillLabels(lbls, sensorName, code, labels);
    return RegisterSensor(new TSolomonSensor(Registry_.Rate(lbls)));
}


ISensorPtr TSolomonBackend::CreateHistogramSensor(const TString& sensorName, TStringBuf code, TLabels labels) {
    NMonitoring::TLabels lbls;
    FillLabels(lbls, sensorName, code, labels);
    return RegisterSensor(new TSolomonSensor(Registry_.HistogramRate(lbls, NMonitoring::ExplicitHistogram(Buckets_))));
}


TString  TSolomonBackend::BuildSensorName(TStringBuf scope, TStringBuf sensor, TStringBuf code, TStringBuf backend, TLabels labels) {
    Y_UNUSED(code, labels);

    TString ret(scope ? scope : TStringBuf("service"));

    ret += '.';
    ret += (backend ? backend : "self");
    ret += '.';
    ret += sensor;

    return ret;
}


bool TSolomonBackend::SerializeMetrics(IOutputStream& stream, EOutputFormat format) {
    NMonitoring::IMetricEncoderPtr encoder = nullptr;

    switch (format) {
        case EOutputFormat::JSON: {
            encoder = NMonitoring::EncoderJson(&stream);
            break;
        }
        case EOutputFormat::SPACK: {
            encoder = NMonitoring::EncoderSpackV1(&stream, NMonitoring::ETimePrecision::SECONDS, NMonitoring::ECompression::IDENTITY);
            break;
        }
        case EOutputFormat::SPACK_LZ4: {
            encoder = NMonitoring::EncoderSpackV1(&stream, NMonitoring::ETimePrecision::SECONDS, NMonitoring::ECompression::LZ4);
            break;
        }
        default:
            return false;
    }
    Registry_.Accept(TInstant::Zero(), encoder.Get());
    return true;
}


void TSolomonBackend::Reset() {
    Registry_.Reset();
}


TString TSolomonBackend::GetHostCType() const {
    return GetEnv("a_ctype", "dev");
}

}   // namespace NMetrics
}   // namespace NVoice
