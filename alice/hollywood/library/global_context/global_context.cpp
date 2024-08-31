#include "global_context.h"

#include <util/generic/guid.h>

namespace NAlice::NHollywood {

TGlobalContext::TGlobalContext(const TConfig& config, NMetrics::ISensors& sensors,
                               TSensorsDumper& sensorsDumper, const TDumpRequestsModeConfig& dumpRequestsModeConfig)
    : Config_(config)
    , RTLogClient_(Config_.GetRTLog())
    , BaseLogger_(RTLogClient_.CreateLogger({}, false))
    , Sensors_(sensors)
    , FastData_(Config_.GetFastDataPath())
    , DumpRequestsModeConfig_(dumpRequestsModeConfig)
    , SensorsDumper_(sensorsDumper)
{
}

const TConfig& TGlobalContext::Config() const {
    return Config_;
}

TRTLogger& TGlobalContext::BaseLogger() {
    return BaseLogger_;
}

TRTLogger TGlobalContext::CreateLogger(const TString& token) {
    return RTLogClient_.CreateLogger(token, false);
}

void TGlobalContext::ReopenLogs() {
    RTLogClient_.Rotate();
}

NMetrics::ISensors& TGlobalContext::Sensors() {
    return Sensors_;
}

TFastData& TGlobalContext::FastData() {
    return FastData_;
}

const TDumpRequestsModeConfig& TGlobalContext::DumpRequestsModeConfig() {
    return DumpRequestsModeConfig_;
}

TSensorsDumper& TGlobalContext::SensorsDumper() {
    return SensorsDumper_;
}

TCommonResources& TGlobalContext::CommonResources() {
    return CommonResources_;
}

} // namespace NAlice::NHollywood
