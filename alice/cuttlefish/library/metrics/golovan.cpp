#include "golovan.h"

#include <library/cpp/unistat/unistat.h>
#include <library/cpp/unistat/types.h>

#include <util/generic/ptr.h>


namespace NVoice {
namespace NMetrics {


class TGolovanSensor : public ISensor {
public:
    TGolovanSensor(NUnistat::IHolePtr ptr)
        : Hole(ptr)
    { }

    void Push(int64_t value) override {
        Hole->PushSignal(value);
    }
private:
    NUnistat::IHolePtr Hole;
};


TGolovanBackend::TGolovanBackend(TAggregationRules rules, const NMonitoring::TBucketBounds& buckets)
    : IBackend(std::move(rules))
{
    for (auto bucket : buckets) {
        Buckets.push_back(bucket);
    }
}


ISensorPtr TGolovanBackend::CreateAbsoluteSensor(const TString& sensorName, TStringBuf code, TLabels labels) {
    const TString name = BuildHoleName(sensorName, code, labels);
    return RegisterSensor(
        new TGolovanSensor(TUnistat::Instance().DrillFloatHole(name, "", "ammx", NUnistat::TPriority(0)))
    );
}


ISensorPtr TGolovanBackend::CreateRateSensor(const TString& sensorName, TStringBuf code, TLabels labels) {
    const TString name = BuildHoleName(sensorName, code, labels);
    return RegisterSensor(
        new TGolovanSensor(TUnistat::Instance().DrillFloatHole(name, "", "dmmm", NUnistat::TPriority(0)))
    );
}


ISensorPtr TGolovanBackend::CreateHistogramSensor(const TString& sensorName, TStringBuf code, TLabels labels) {
    const TString name = BuildHoleName(sensorName, code, labels) + ".mcs";
    return RegisterSensor(
        new TGolovanSensor(TUnistat::Instance().DrillHistogramHole(name, "", "dhhh", NUnistat::TPriority(0), Buckets))
    );
}


TString TGolovanBackend::BuildSensorName(TStringBuf scope, TStringBuf sensor, TStringBuf code, TStringBuf backend, TLabels labels) {
    Y_UNUSED(code, labels);

    TString name;

    if (scope) {
        name += scope;
        name += '.';
    }

    if (backend) {
        name += backend;
        name += '.';
    }

    name += sensor;

    return name;
}


TString TGolovanBackend::BuildHoleName(TStringBuf sensorName, TStringBuf code, const TLabels& labels) {
    TString prefix = "";
    prefix += labels.GroupName;
    prefix += '.';

    prefix += labels.DeviceName;
    prefix += '.';

    if (labels.ClientType == EClientType::User) {
        prefix += "user.";
    } else if (labels.ClientType == EClientType::Robot) {
        prefix += "robot.";
    } else if (labels.ClientType == EClientType::None) {
        prefix += "unknown.";
    }

    if (code) {
        return prefix + sensorName + '.' + code;
    } else {
        return prefix + sensorName;
    }
}


bool TGolovanBackend::SerializeMetrics(IOutputStream& stream, EOutputFormat format) {
    if (format == EOutputFormat::JSON) {
        stream << TUnistat::Instance().CreateJsonDump(0);
    } else if (format == EOutputFormat::TEXT) {
        stream << TUnistat::Instance().CreateInfoDump(0);
    } else {
        return false;
    }
    return true;
}


void TGolovanBackend::Reset() {
    TUnistat::Instance().ResetSignals();
}


}   // namespace NMetrics
}   // namespace NVoice
