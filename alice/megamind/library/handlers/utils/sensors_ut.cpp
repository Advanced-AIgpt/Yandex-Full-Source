#include "sensors.h"

#include <alice/library/logger/logger.h>
#include <alice/library/unittest/mock_sensors.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace testing;
using namespace NAlice;
using namespace NAlice::NMegamind;

Y_UNIT_TEST_SUITE(HandlersUtilsSensors) {
    Y_UNIT_TEST(RequestTimeMetricsSmoke) {
        TMockSensors sensors;
        auto& log = TRTLogger::NullLogger();

        THashSet<TString> storage;
        auto onAddHistogram = [&storage](const auto& labels, const auto& /* value */, const auto& /* intervals */) {
            storage.emplace(ToString(labels));
        };
        EXPECT_CALL(sensors, AddHistogram(_, _, _)).WillRepeatedly(Invoke(onAddHistogram));

        {
            TRequestTimeMetrics testMetrics{sensors, log, "node", EUniproxyStage::Run, ERequestResult::Success};
        }
        UNIT_ASSERT_C(storage.empty(), "must be empty because no start time");

        {
            TRequestTimeMetrics testMetrics{sensors, log, "node", EUniproxyStage::Run, ERequestResult::Success};
            testMetrics.SetStartTime(TInstant::Now());
        }
        UNIT_ASSERT_C(storage.count("{name=handler.run_time_milliseconds, result=success, client=, path=, node=node}"), "Just a regular add");

        {
            storage.clear();
            TRequestTimeMetrics testMetrics{sensors, log, "node", EUniproxyStage::Run, ERequestResult::Success};
            testMetrics.UpdateResult(ERequestResult::Fail);
            testMetrics.SetStartTime(TInstant::Now());
        }
        UNIT_ASSERT_C(storage.count("{name=handler.run_time_milliseconds, result=fail, client=, path=, node=node}"), "UpdateResult Fail");

        {
            storage.clear();
            TRequestTimeMetrics testMetrics{sensors, log, "node", EUniproxyStage::Apply, ERequestResult::Success};
            testMetrics.SetStartTime(TInstant::Now());
            testMetrics.SetSkrInfo("clientName", "requestPath");
        }
        UNIT_ASSERT_C(storage.count("{name=handler.apply_time_milliseconds, result=success, client=clientName, path=requestPath, node=node}"), "skrinfo");
    }
}

} // namespace
