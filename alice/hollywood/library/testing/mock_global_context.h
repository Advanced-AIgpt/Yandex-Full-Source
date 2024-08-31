#pragma once

#include <alice/hollywood/library/config/config.pb.h>
#include <alice/hollywood/library/context/context.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg.h>

#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <alice/library/logger/logger.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/metrics/sensors_dumper/sensors_dumper.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice::NHollywood {

class TMockGlobalContext : public IGlobalContext {
public:
    MOCK_METHOD(const TConfig&, Config, (), (const, override));
    MOCK_METHOD(TRTLogger&, BaseLogger, (), (override));
    MOCK_METHOD(TRTLogger, CreateLogger, (const TString&), (override));
    MOCK_METHOD(void, ReopenLogs, (), (override));
    MOCK_METHOD(NMetrics::ISensors&, Sensors, (), (override));
    MOCK_METHOD(TFastData&, FastData, (), (override));
    MOCK_METHOD(const TDumpRequestsModeConfig&, DumpRequestsModeConfig, (), (override));
    MOCK_METHOD(TSensorsDumper&, SensorsDumper, (), (override));
    MOCK_METHOD(TCommonResources&, CommonResources, (), (override));
};


} // namespace NAlice
