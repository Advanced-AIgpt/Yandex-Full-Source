#pragma once

#include <alice/hollywood/library/config/config.pb.h>
#include <alice/hollywood/library/fast_data/fast_data.h>
#include <alice/hollywood/library/resources/common_resources.h>

#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <alice/library/logger/logger.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/metrics/sensors_dumper/sensors_dumper.h>

#include <apphost/api/service/cpp/service_context.h>

#include <util/generic/noncopyable.h>

namespace NAlice::NHollywood {

struct TDumpRequestsModeConfig {
    bool Enabled = false;
    TString OutputDirPath;
};

class IGlobalContext {
public:
    virtual ~IGlobalContext() = default;

    virtual const TConfig& Config() const = 0;
    virtual TRTLogger& BaseLogger() = 0;
    virtual TRTLogger CreateLogger(const TString& token) = 0;
    virtual void ReopenLogs() = 0;
    virtual NMetrics::ISensors& Sensors() = 0;
    virtual TFastData& FastData() = 0;
    virtual const TDumpRequestsModeConfig& DumpRequestsModeConfig() = 0;
    virtual TSensorsDumper& SensorsDumper() = 0;
    virtual TCommonResources& CommonResources() = 0;
};

class TGlobalContext : public IGlobalContext, private NNonCopyable::TNonCopyable {
public:
    TGlobalContext(const TConfig& config, NMetrics::ISensors& sensors,
                   TSensorsDumper& sensorsDumper, const TDumpRequestsModeConfig& dumpRequestsModeConfig={});

    const TConfig& Config() const override;
    TRTLogger& BaseLogger() override;
    TRTLogger CreateLogger(const TString& token) override;
    void ReopenLogs() override;
    NMetrics::ISensors& Sensors() override;
    TFastData& FastData() override;
    const TDumpRequestsModeConfig& DumpRequestsModeConfig() override;
    TSensorsDumper& SensorsDumper() override;
    TCommonResources& CommonResources() override;

private:
    const TConfig& Config_;
    TRTLogClient RTLogClient_;
    TRTLogger BaseLogger_;
    NMetrics::ISensors& Sensors_;
    TFastData FastData_;
    TDumpRequestsModeConfig DumpRequestsModeConfig_;
    TSensorsDumper& SensorsDumper_;
    TCommonResources CommonResources_;
};

} // namespace NAlice::NHollywood
