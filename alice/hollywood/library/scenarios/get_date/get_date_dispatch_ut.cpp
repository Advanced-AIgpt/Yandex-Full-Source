#include "get_date.h"

#include <alice/hollywood/library/scenarios/get_date/proto/get_date.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NGetDate {

namespace {

const TStringBuf GET_DATE1 = "["
                                 "{\"name\":\"query_target\",\"type\":\"string\",\"value\":\"day_of_week\"}"
                             "]";


} // anonimous namespace

Y_UNIT_TEST_SUITE(GetDateDispatch) {

    Y_UNIT_TEST(GetDateDispatch1) {
        TTestEnvironment testData("get_date", "ru-ru");
        testData.AddSemanticFrame(FRAME_GET_DATE, GET_DATE1);

        UNIT_ASSERT(testData >> TTestDispatch(&TGetDateScenario::Dispatch) >> testData);

        UNIT_ASSERT(testData.GetSelectedSceneName() == SCENE_NAME_DEFAULT);
        UNIT_ASSERT(testData.GetSelectedIntent() == FRAME_GET_DATE);
        const google::protobuf::Any& args = testData.GetSelectedSceneArguments();
        TGetDateSceneArgs sceneArgs;
        UNIT_ASSERT(args.UnpackTo(&sceneArgs));
        UNIT_ASSERT_STRINGS_EQUAL(sceneArgs.GetQueryTarget(), "day_of_week");
    }
}

} // namespace NAlice::NHollywoodFw::NGetDate
