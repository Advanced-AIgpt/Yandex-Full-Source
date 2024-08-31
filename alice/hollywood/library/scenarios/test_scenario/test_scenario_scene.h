#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/test_scenario/proto/test_scenario.pb.h>

namespace NAlice::NHollywoodFw::NTestScenario {

inline constexpr TStringBuf FAKE_SCENE_NAME_DEFAULT = "fake_test_scene";

/*
    Fake scene for TestScenario
    This scene is never called because TestScenario always returns irrelevant answer
*/
class TTestScenarioFakeScene : public TScene<TTestScenarioFakeSceneArgs> {
public:
TTestScenarioFakeScene(const TScenario* owner);

    TRetMain Main(const TTestScenarioFakeSceneArgs& args,
                  const TRunRequest& runRequest,
                  TStorage& storage,
                  const TSource& source) const override;
};

}  // namespace NAlice::NHollywood::NTestScenario
