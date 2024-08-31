#include "bluetooth.h"
#include "bluetooth_scene.h"

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/hollywood/library/scenarios/bluetooth/proto/bluetooth.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NBluetooth {

namespace {

const TStringBuf FRAME = "[]";


} // anonimous namespace

Y_UNIT_TEST_SUITE(BluetoothDispatch) {

    Y_UNIT_TEST(BluetoothDispatchOn) {
        TTestEnvironment testData(NProductScenarios::BLUETOOTH, "ru-ru");
        testData.AddSemanticFrame(FRAME_BLUETOOTH_ON, FRAME);
        testData.RunRequest.MutableBaseRequest()->MutableInterfaces()->SetHasBluetooth(true);

        UNIT_ASSERT(testData >> TTestDispatch(&TBluetoothScenario::Dispatch) >> testData);

        UNIT_ASSERT(testData.GetSelectedSceneName() == SCENE_NAME_ON);
        UNIT_ASSERT(testData.GetSelectedIntent() == FRAME_BLUETOOTH_ON);
    }

    Y_UNIT_TEST(BluetoothDispatchOff) {
        TTestEnvironment testData(NProductScenarios::BLUETOOTH, "ru-ru");
        testData.AddSemanticFrame(FRAME_BLUETOOTH_OFF, FRAME);
        testData.RunRequest.MutableBaseRequest()->MutableInterfaces()->SetHasBluetooth(true);

        UNIT_ASSERT(testData >> TTestDispatch(&TBluetoothScenario::Dispatch) >> testData);

        UNIT_ASSERT(testData.GetSelectedSceneName() == SCENE_NAME_OFF);
        UNIT_ASSERT(testData.GetSelectedIntent() == FRAME_BLUETOOTH_OFF);
    }

    Y_UNIT_TEST(BluetoothDispatchUnsupported) {
        TTestEnvironment testData(NProductScenarios::BLUETOOTH, "ru-ru");
        testData.AddSemanticFrame(FRAME_BLUETOOTH_ON, FRAME);
        testData.RunRequest.MutableBaseRequest()->MutableInterfaces()->SetHasBluetooth(false);

        UNIT_ASSERT(testData >> TTestDispatch(&TBluetoothScenario::Dispatch) >> testData);

        UNIT_ASSERT(testData.GetSelectedSceneName() == SCENE_NAME_UNSUPPORTED);
        UNIT_ASSERT(testData.GetSelectedIntent() == FRAME_BLUETOOTH_ON);
    }
}

} // namespace NAlice::NHollywoodFw::NBluetooth
