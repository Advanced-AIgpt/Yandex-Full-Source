#include "order.h"
#include "order_scene.h"
#include "order_status_notification_scene.h"

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/hollywood/library/scenarios/order/proto/order.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NOrder {

namespace {

const TStringBuf FRAME = "[]";


} // anonimous namespace

Y_UNIT_TEST_SUITE(OrderDispatch) {

    Y_UNIT_TEST(OrderDispatch) {
        TTestEnvironment testData(NProductScenarios::ORDER, "ru-ru");
        testData.AddPUID("123456");
        testData.AddSemanticFrame(FRAME_ORDER, FRAME);

        UNIT_ASSERT(testData >> TTestDispatch(&TOrderScenario::Dispatch) >> testData);

        UNIT_ASSERT(testData.GetSelectedSceneName() == SCENE_NAME_ORDER);
        UNIT_ASSERT(testData.GetSelectedIntent() == FRAME_ORDER);
        const google::protobuf::Any& args = testData.GetSelectedSceneArguments();
        TOrderSceneArgs sceneArgs;
        UNIT_ASSERT(args.UnpackTo(&sceneArgs));
    }

    Y_UNIT_TEST(Notification) {
        TTestEnvironment testData(NProductScenarios::ORDER, "ru-ru");
        testData.AddPUID("123456");
        testData.AddSemanticFrame(FRAME_ORDER_NOTIFICATION, FRAME);

        UNIT_ASSERT(testData >> TTestDispatch(&TOrderScenario::Dispatch) >> testData);

        UNIT_ASSERT(testData.GetSelectedSceneName() == SCENE_NAME_NOTIFICATION);
        UNIT_ASSERT(testData.GetSelectedIntent() == FRAME_ORDER_NOTIFICATION);
        const google::protobuf::Any& args = testData.GetSelectedSceneArguments();
        TOrderStatusNotificationSceneArgs sceneArgs;
        UNIT_ASSERT(args.UnpackTo(&sceneArgs));
    }

    Y_UNIT_TEST(OrderIrrelevant) {
        TTestEnvironment testData(NProductScenarios::ORDER, "ru-ru");
        testData.AddPUID("123456");

        UNIT_ASSERT(testData >> TTestDispatch(&TOrderScenario::Dispatch) >> testData);
        UNIT_ASSERT(testData.IsIrrelevant());
    }
}

} // namespace NAlice::NHollywoodFw::NOrder