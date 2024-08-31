#include "bluetooth_scene.h"

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/hollywood/library/scenarios/bluetooth/proto/bluetooth.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NBluetooth {

namespace {


} // anonimous namespace

Y_UNIT_TEST_SUITE(BluetoothRender) {

    Y_UNIT_TEST(BluetoothRenderOn) {
        TTestEnvironment testData(NProductScenarios::BLUETOOTH, "ru-ru");

        UNIT_ASSERT(testData >> TTestRender(&TBluetoothSceneOn::Render, {}) >> testData);
        UNIT_ASSERT(!testData.IsIrrelevant());
        UNIT_ASSERT(testData.GetText());
        UNIT_ASSERT(testData.GetVoice());

        const auto& directives = testData.ResponseBody.GetLayout().GetDirectives();
        UNIT_ASSERT_VALUES_EQUAL(directives.size(), 1);
        UNIT_ASSERT(directives[0].HasStartBluetoothDirective());
    }

    Y_UNIT_TEST(BluetoothRenderOff) {
        TTestEnvironment testData(NProductScenarios::BLUETOOTH, "ru-ru");

        UNIT_ASSERT(testData >> TTestRender(&TBluetoothSceneOff::Render, {}) >> testData);
        UNIT_ASSERT(!testData.IsIrrelevant());
        UNIT_ASSERT(testData.GetText());
        UNIT_ASSERT(testData.GetVoice());

        const auto& directives = testData.ResponseBody.GetLayout().GetDirectives();
        UNIT_ASSERT_VALUES_EQUAL(directives.size(), 1);
        UNIT_ASSERT(directives[0].HasStopBluetoothDirective());
    }

    Y_UNIT_TEST(BluetoothRenderUnsupported) {
        TTestEnvironment testData(NProductScenarios::BLUETOOTH, "ru-ru");

        UNIT_ASSERT(testData >> TTestRender(&TBluetoothSceneUnsupported::Render, {}) >> testData);
        UNIT_ASSERT(!testData.IsIrrelevant());
        UNIT_ASSERT(testData.GetText());
        UNIT_ASSERT(testData.GetVoice());

        const auto& directives = testData.ResponseBody.GetLayout().GetDirectives();
        UNIT_ASSERT_VALUES_EQUAL(directives.size(), 0);
    }
}

} // namespace NAlice::NHollywoodFw::NBluetooth
