#include "config.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {
Y_UNIT_TEST_SUITE(MonitoringPort) {
    Y_UNIT_TEST(Smoke) {
        {
            TConfig config;
            UNIT_CHECK_GENERATED_EXCEPTION(MonitoringPort(config), yexception);
        }

        {
            TConfig config;
            auto* monService = config.MutableMonService();
            monService->SetPort(100000);
            UNIT_CHECK_GENERATED_EXCEPTION(MonitoringPort(config), yexception);
        }

        {
            TConfig config;
            auto* monService = config.MutableMonService();
            monService->SetPort(1);
            UNIT_ASSERT_VALUES_EQUAL(MonitoringPort(config), 1);

            monService->SetPort(65534);
            UNIT_ASSERT_VALUES_EQUAL(MonitoringPort(config), 65534);
        }

        {
            TConfig config;
            auto* monService = config.MutableMonService();
            monService->SetPortOffset(1);
            UNIT_CHECK_GENERATED_EXCEPTION(MonitoringPort(config), yexception);

            auto* server = config.MutableAppHost();
            server->SetHttpPort(42);
            UNIT_ASSERT_VALUES_EQUAL(MonitoringPort(config), 43);

            server->SetHttpPort(65534);
            UNIT_CHECK_GENERATED_EXCEPTION(MonitoringPort(config), yexception);
        }
    }
}
}  // namespace
