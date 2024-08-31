#include "metronome.h"

#include <alice/hollywood/library/scenarios/metronome/proto/metronome_render_state.pb.h>
#include <alice/hollywood/library/scenarios/metronome/proto/metronome_scenario_state.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/util.h>

namespace NAlice::NHollywoodFw::NMetronome {

Y_UNIT_TEST_SUITE(TestMetronome) {

    Y_UNIT_TEST(MetronomeDispatchToStart) {
        {
            TTestEnvironment testData("metronome", "ru-ru");
            testData.AddSemanticFrame("alice.metronome.start", "{}");
            UNIT_ASSERT(testData >> TTestDispatch(&TMetronomeScenario::Dispatch) >> testData);
            UNIT_ASSERT(testData.GetSelectedSceneName() == SCENE_NAME_START);
            TMetronomeScenarioStartArguments state;
            testData.GetSelectedSceneArguments().UnpackTo(&state);
            UNIT_ASSERT(!state.HasBpm());
        }
        {
            TTestEnvironment testData("metronome", "ru-ru");
            auto frameJson = TStringBuf(R"(
            [
                {
                    "name": "bpm",
                    "value": "123",
                    "type": "sys.num"
                }
            ]
            )");
            testData.AddSemanticFrame("alice.metronome.start", frameJson);
            UNIT_ASSERT(testData >> TTestDispatch(&TMetronomeScenario::Dispatch) >> testData);
            UNIT_ASSERT(testData.GetSelectedSceneName() == SCENE_NAME_START);
            TMetronomeScenarioStartArguments state;
            testData.GetSelectedSceneArguments().UnpackTo(&state);
            UNIT_ASSERT(state.GetBpm() == 123);
        }
    }

    Y_UNIT_TEST(MetronomeDispatchToUpdate) {
        THashMap<TString, TMetronomeScenarioUpdateArguments::EMethod> cases = {
            {"alice.metronome.faster", TMetronomeScenarioUpdateArguments::Faster},
            {"alice.metronome.slower", TMetronomeScenarioUpdateArguments::Slower}
        };
        TTestEnvironment testData("metronome", "ru-ru");
        for (const auto& [frameName, method] : cases) {
            testData.AddSemanticFrame(frameName, "{}");
            UNIT_ASSERT(testData >> TTestDispatch(&TMetronomeScenario::Dispatch) >> testData);
            UNIT_ASSERT(testData.GetSelectedSceneName() == SCENE_NAME_UPDATE);
            TMetronomeScenarioUpdateArguments state;
            testData.GetSelectedSceneArguments().UnpackTo(&state);
            UNIT_ASSERT(state.GetMethod() == method);
        }
    }

    Y_UNIT_TEST(MetronomeStartRender) {
        THashMap<TString, TString> cases = {
            {"exact", "Включаю. 100 bpm."},
            {"max", "Максимум 100 bpm. Включаю."},
            {"min", "Минимум 100 bpm. Включаю."}
        };
        TTestEnvironment testData("metronome", "ru-ru");

        for (const auto& [responseType, answer] : cases) {
            TMetronomeRenderStartArguments renderState;
            renderState.SetBpm(100);
            renderState.SetResponseType(responseType);
            UNIT_ASSERT(testData >> TTestRender(&TMetronomeScenario::RenderStartScene, renderState) >> testData);
            UNIT_ASSERT(testData.ResponseBody.GetLayout().GetOutputSpeech() == answer);
        }
    }

    Y_UNIT_TEST(MetronomeUpdateRender) {
        THashMap<TString, TString> cases = {
            {"exact", "100 bpm."},
            {"max", "Максимум 100 bpm. Включаю."},
            {"min", "Минимум 100 bpm. Включаю."}
        };
        TTestEnvironment testData("metronome", "ru-ru");

        for (const auto& [responseType, answer] : cases) {
            TMetronomeRenderUpdateArguments renderState;
            renderState.SetBpm(100);
            renderState.SetResponseType(responseType);
            UNIT_ASSERT(testData >> TTestRender(&TMetronomeScenario::RenderUpdateScene, renderState) >> testData);
            UNIT_ASSERT(testData.ResponseBody.GetLayout().GetOutputSpeech() == answer);
        }
    }
}

} // namespace NAlice::NHollywoodFw::NMetronome
