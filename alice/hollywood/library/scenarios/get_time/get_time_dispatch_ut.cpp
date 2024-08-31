#include "get_time.h"
#include "vins_generic_scene.h"

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/hollywood/library/scenarios/get_time/proto/get_time.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NGetTime {

Y_UNIT_TEST_SUITE(GetTimeDispatch) {

    Y_UNIT_TEST(GetTimeDispatch) {
        TTestEnvironment testData(NProductScenarios::GET_TIME, "ru-ru");
        UNIT_ASSERT(testData >> TTestDispatch(&TGetTimeScenario::Dispatch) >> testData);

        UNIT_ASSERT(testData.GetSelectedSceneName() == SCENE_NAME_VINS_GENERIC);
        const google::protobuf::Any& args = testData.GetSelectedSceneArguments();
        UNIT_ASSERT(args.Is<TGetTimeVinsGenericSceneArgs>());
    }
}

} // namespace NAlice::NHollywoodFw::NGetTime
