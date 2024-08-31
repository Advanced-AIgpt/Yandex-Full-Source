#include "{{scenario_name}}.h"
#include "{{scenario_name}}_scene.h"

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/hollywood/library/scenarios/{{scenario_name}}/proto/{{scenario_name}}.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::N{{ScenarioName}} {

namespace {

const TStringBuf FRAME = "["
    "{\"name\":\"nominal\",\"type\":\"string\",\"value\":\"World\"}"
"]";


} // anonimous namespace

Y_UNIT_TEST_SUITE({{ScenarioName}}Dispatch) {

    Y_UNIT_TEST({{ScenarioName}}Dispatch) {
        TTestEnvironment testData(NProductScenarios::{{SCENARIO_NAME}}, "ru-ru");
        testData.AddSemanticFrame(FRAME_{{FRAME_NAME}}, FRAME);

        UNIT_ASSERT(testData >> TTestDispatch(&T{{ScenarioName}}Scenario::Dispatch) >> testData);

        UNIT_ASSERT(testData.GetSelectedSceneName() == SCENE_NAME_DEFAULT);
        UNIT_ASSERT(testData.GetSelectedIntent() == FRAME_{{FRAME_NAME}});
        const google::protobuf::Any& args = testData.GetSelectedSceneArguments();
        T{{ScenarioName}}SceneArgs sceneArgs;
        UNIT_ASSERT(args.UnpackTo(&sceneArgs));
        UNIT_ASSERT_STRINGS_EQUAL(sceneArgs.GetName(), "World");
        UNIT_ASSERT(sceneArgs.GetAge() == 42);
    }
}

} // namespace NAlice::NHollywoodFw::N{{ScenarioName}}
