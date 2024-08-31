#include "sensors_dumper.h"

#include <library/cpp/monlib/encode/encoder.h>
#include <library/cpp/monlib/encode/json/json_encoder.cpp>
#include <library/cpp/unistat/unistat.h>

namespace NAlice {

TSensorsDumper::TSensorsDumper(NMonitoring::TMetricRegistry& solomonSensors)
    : SolomonSensors(solomonSensors)
{
}

TString TSensorsDumper::Dump(TStringBuf format) {
    if (format == "solomon") {
        TStringStream out;
        NMonitoring::IMetricEncoderPtr encoder = NMonitoring::EncoderJson(&out);
        SolomonSensors.Accept(TInstant::Zero(), encoder.Get());
        encoder->Close();
        return out.Str();
    }
    return {};
}

} // namespace NAlice
