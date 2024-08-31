#pragma once

#include "test_environment.h"

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/global_context/global_context.h>

#include <alice/library/metrics/sensors_dumper/sensors_dumper.h>
#include <alice/library/unittest/mock_sensors.h>

namespace NAlice::NHollywoodFw::NPrivate {

//
// Mock fast data with direct access to internal fastdata object
//
class TMockFastData : public NHollywood::TFastData {
public:
    TMockFastData()
        : TFastData("")
    {
    }
    TVector<NHollywood::TScenarioFastDataPtr>& GetFastData() {
        return FastData_;
    }
};

class TTestGlobalContext: public NHollywood::IGlobalContext {
public:
    TTestGlobalContext(const TTestEnvironment& env)
        : SensorsDumper_(MetricRegistry_)
    {
        // Attach all existing fast data to mock context
        for (const auto& it : env.GetFastData()) {
            FastData_.GetFastData().push_back(it);
        }
    }

    const NHollywood::TConfig& Config() const override {
        return Config_;
    }
    TRTLogger& BaseLogger() override {
        return TRTLogger::StderrLogger();
    }
    TRTLogger CreateLogger(const TString& token) override {
        Y_UNUSED(token);
        return std::move(TRTLogger::StderrLogger());
    }
    void ReopenLogs() override {
        // noop
    }
    NMetrics::ISensors& Sensors() override {
        return NoopSensors_;
    }
    NHollywood::TFastData& FastData() override {
        return FastData_;
    }
    NHollywood::TDumpRequestsModeConfig& DumpRequestsModeConfig() override {
        return DumpRequestsModeConfig_;
    }
    TSensorsDumper& SensorsDumper() override {
        return SensorsDumper_;
    }
    NHollywood::TCommonResources& CommonResources() override {
        return CommonResources_;
    }
private:
    TMockFastData FastData_;
    NHollywood::TCommonResources CommonResources_;

    NHollywood::TConfig Config_;
    TNoopSensors NoopSensors_;
    NHollywood::TDumpRequestsModeConfig DumpRequestsModeConfig_;
    NMonitoring::TMetricRegistry MetricRegistry_;
    TSensorsDumper SensorsDumper_;
};

} // namespace NAlice::NHollywoodFw::NPrivate
