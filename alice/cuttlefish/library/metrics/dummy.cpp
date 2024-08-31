#include "dummy.h"

#include <util/stream/str.h>


namespace NVoice {
namespace NMetrics {


class TDummySensorImpl : public TThrRefBase {
public:
    TDummySensorImpl(TStringBuf name, TStringBuf code, TLabels labels, TStringBuf type, bool logging);

    void Push(int64_t value);

    TString ToString() const;

public:
    int64_t     Value = 0;
    TString     Name;
    TString     Type;
    TString     Code;
    TClientInfo Labels;
    bool        Logging { false };
};


TDummySensorImpl::TDummySensorImpl(TStringBuf name, TStringBuf code, TLabels labels, TStringBuf type, bool logging)
    : Name(name)
    , Type(type)
    , Code(code)
    , Logging(logging)
{
    Labels.From(labels);
}


void TDummySensorImpl::Push(int64_t value) {
    Value += value;
    if (Logging) {
        Cerr << "TDummySensor::Push(" << ToString() << ")" << Endl;
    }
}


TString TDummySensorImpl::ToString() const {
    TStringStream oss;
    oss << "sensor=" << Name << "\t"
        << "type=" << Type << "\t"
        << "code=" << Code << "\t"
        << "device=" << Labels.DeviceName << "\t"
        << "surface=" << Labels.GroupName << "\t"
        << "surface_type=" << Labels.SubgroupName << "\t"
        << "app=" << Labels.AppId << "\t"
        << "client_type=" << (Labels.ClientType == EClientType::User ? "user" : (Labels.ClientType == EClientType::Robot ? "robot" : "")) << "\t"
        << "value=" << Value
    ;
    return oss.Str();
}


class TDummySensor : public ISensor {
public:
    TDummySensor(TIntrusivePtr<TDummySensorImpl> ptr)
        : Ptr(ptr)
    { }

    void Push(int64_t value) override {
        Ptr->Push(value);
    }

private:
    TIntrusivePtr<TDummySensorImpl> Ptr;
};


TDummyBackend::TDummyBackend(TAggregationRules rules, const NMonitoring::TBucketBounds& buckets, bool logging)
    : IBackend(std::move(rules))
    , Logging(logging)
{
    Y_UNUSED(buckets);
}


TDummyBackend::~TDummyBackend()
{ }


ISensorPtr TDummyBackend::CreateSensor(const TString& sensorName, TStringBuf code, TLabels labels, TStringBuf type, bool logging) {
    TIntrusivePtr<TDummySensorImpl> sensor = MakeIntrusive<TDummySensorImpl>(sensorName, code, labels, type, logging);
    Sensors_.emplace_back(sensor);
    return RegisterSensor(
        new TDummySensor(sensor)
    );
}


ISensorPtr TDummyBackend::CreateAbsoluteSensor(const TString& sensorName, TStringBuf code, TLabels labels) {
    if (Logging) {
        Cerr << "TDummyBackend::CreateAbsoluteSensor("
            << "Name='" << sensorName
            << "', Code='" << code
            << "', Device='" << labels.DeviceName
            << "', Surface='" << labels.GroupName
            << "', SubgroupName='" << labels.SubgroupName
            << "', AppId='" << labels.AppId
            << "')" << Endl;
    }
    return CreateSensor(sensorName, code, labels, "gauge", Logging);
}


ISensorPtr TDummyBackend::CreateRateSensor(const TString& sensorName, TStringBuf code, TLabels labels) {
    if (Logging) {
        Cerr << "TDummyBackend::CreateRateSensor("
            << "sensor='" << sensorName
            << "', code='" << code
            << "', device='" << labels.DeviceName
            << "', surface='" << labels.GroupName
            << "', surface_type='" << labels.SubgroupName
            << "', app_id='" << labels.AppId
            << "')" << Endl;
    }
    return CreateSensor(sensorName, code, labels, "rate", Logging);
}


ISensorPtr TDummyBackend::CreateHistogramSensor(const TString& sensorName, TStringBuf code, TLabels labels) {
    if (Logging) {
        Cerr << "TDummyBackend::CreateHistogramSensor("
            << "sensor='" << sensorName
            << "', code='" << code
            << "', device='" << labels.DeviceName
            << "', surface='" << labels.GroupName
            << "', surface_type='" << labels.SubgroupName
            << "', app_id='" << labels.AppId
            << "')" << Endl;
    }
    return CreateSensor(sensorName, code, labels, "hist", Logging);
}


TString  TDummyBackend::BuildSensorName(TStringBuf scope, TStringBuf sensor, TStringBuf code, TStringBuf backend, TLabels labels) {
    Y_UNUSED(code, labels);

    TString ret(scope ? scope : TStringBuf("service"));

    ret += '.';
    ret += (backend ? backend : "self");
    ret += '.';
    ret += sensor;

    return ret;
}


bool TDummyBackend::SerializeMetrics(IOutputStream& stream, EOutputFormat format) {
    if (format == EOutputFormat::TEXT) {
        for (const auto& sens : Sensors_) {
            stream << sens->ToString() << Endl;
        }
    } else if (format == EOutputFormat::JSON) {
        stream << "{}";
    } else {
        return false;
    }

    return false;
}


}   // namespace NMetrics
}   // namespace NVoice
