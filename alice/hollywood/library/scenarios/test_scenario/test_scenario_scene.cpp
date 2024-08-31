#include "test_scenario_scene.h"

#include <alice/hollywood/library/scenarios/test_scenario/proto/test_scenario.pb.h>

namespace NAlice::NHollywoodFw::NTestScenario {

TTestScenarioFakeScene::TTestScenarioFakeScene(const TScenario* owner)
    : TScene(owner, FAKE_SCENE_NAME_DEFAULT)
{
}

TRetMain TTestScenarioFakeScene::Main(
        const TTestScenarioFakeSceneArgs& args,
        const TRunRequest& runRequest,
        TStorage& storage,
        const TSource& source) const
{
    Y_UNUSED(args);
    Y_UNUSED(runRequest);
    Y_UNUSED(storage);
    Y_UNUSED(source);
    HW_ERROR("This scene is never called");
}

}  // namespace NAlice::NHollywood::NTestScenario
