#include "music_what_is_playing.h"

#include <alice/hollywood/library/scenarios/music_what_is_playing/nlg/register.h>
#include <alice/hollywood/library/scenarios/music_what_is_playing/vins_scene.h>
#include <alice/hollywood/library/vins/hwf_state.h>
#include <alice/library/analytics/common/product_scenarios.h>

namespace NAlice::NHollywoodFw::NMusicWhatIsPlaying {

HW_REGISTER(TMusicWhatIsPlayingScenario);

TMusicWhatIsPlayingScenario::TMusicWhatIsPlayingScenario()
    : TScenario(NProductScenarios::MUSIC_WHAT_IS_PLAYING)
{
    Register(&TMusicWhatIsPlayingScenario::Dispatch);
    RegisterScene<TMusicWhatIsPlayingVinsScene>([this]() {
        RegisterSceneFn(&TMusicWhatIsPlayingVinsScene::MainSetup);
        RegisterSceneFn(&TMusicWhatIsPlayingVinsScene::Main);
        RegisterSceneFn(&TMusicWhatIsPlayingVinsScene::ApplySetup);
        RegisterSceneFn(&TMusicWhatIsPlayingVinsScene::Apply);
    });

    SetApphostGraph(ScenarioRequest() >> NodeRun("run_setup") >> NodeMain("run_main") >> ScenarioResponse());
    SetApphostGraph(ScenarioApply() >> NodeApplySetup("apply_setup") >> NodeApply("apply_main") >> ScenarioResponse());

    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NMusicWhatIsPlaying::NNlg::RegisterAll);
}

TRetScene TMusicWhatIsPlayingScenario::Dispatch(const TRunRequest&, const TStorage&, const TSource&) const {
    return TReturnValueScene<TMusicWhatIsPlayingVinsScene>(TMusicWhatIsPlayingVinsSceneArgs());
}

void TMusicWhatIsPlayingScenario::Hook(THookInputInfo& info, NScenarios::TScenarioRunResponse& runResponse) const {
    TMusicWhatIsPlayingVinsRunRenderArgs musicWhatIsPlayingsRunRenderArgs;
    if (info.RenderArguments && info.RenderArguments->UnpackTo(&musicWhatIsPlayingsRunRenderArgs)) {
        SaveHwfState(runResponse, *musicWhatIsPlayingsRunRenderArgs.MutableScenarioRunResponse());
        runResponse = std::move(*musicWhatIsPlayingsRunRenderArgs.MutableScenarioRunResponse());
    }
}

void TMusicWhatIsPlayingScenario::Hook(THookInputInfo& info, NScenarios::TScenarioApplyResponse& applyResponse) const {
    TMusicWhatIsPlayingVinsApplyRenderArgs musicWhatIsPlayingsApplyRenderArgs;
    if (info.RenderArguments && info.RenderArguments->UnpackTo(&musicWhatIsPlayingsApplyRenderArgs)) {
        SaveHwfState(applyResponse, *musicWhatIsPlayingsApplyRenderArgs.MutableScenarioApplyResponse());
        applyResponse = std::move(*musicWhatIsPlayingsApplyRenderArgs.MutableScenarioApplyResponse());
    }
}

}  // namespace NAlice::NHollywood::NMusicWhatIsPlaying
