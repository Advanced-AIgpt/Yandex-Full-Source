#include "get_time.h"
#include "vins_generic_scene.h"

#include <alice/hollywood/library/scenarios/get_time/nlg/register.h>
#include <alice/hollywood/library/vins/hwf_state.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/library/analytics/common/product_scenarios.h>

namespace NAlice::NHollywoodFw::NGetTime {

HW_REGISTER(TGetTimeScenario);

TGetTimeScenario::TGetTimeScenario()
    : TScenario(NProductScenarios::GET_TIME)
{
    Register(&TGetTimeScenario::Dispatch);
    RegisterScene<TVinsGenericScene>([this]() {
        RegisterSceneFn(&TVinsGenericScene::MainSetup);
        RegisterSceneFn(&TVinsGenericScene::Main);
    });

    SetApphostGraph(ScenarioRequest() >>
                    NodeRun("run_prepare") >>
                    NodeMain("run_parse") >>
                    ScenarioResponse());

    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NGetTime::NNlg::RegisterAll);
}

TRetScene TGetTimeScenario::Dispatch(const TRunRequest&, const TStorage&, const TSource&) const {
    return TReturnValueScene<TVinsGenericScene>(TGetTimeVinsGenericSceneArgs());
}

void TGetTimeScenario::Hook(THookInputInfo& info, NScenarios::TScenarioRunResponse& runResponse) const {
    TGetTimeVinsGenericRenderArgs vinsGenericRenderArgs;
    if (info.RenderArguments && info.RenderArguments->UnpackTo(&vinsGenericRenderArgs)) {
        SaveHwfState(runResponse, *vinsGenericRenderArgs.MutableScenarioRunResponse());
        runResponse = std::move(*vinsGenericRenderArgs.MutableScenarioRunResponse());
    }
}

}  // namespace NAlice::NHollywood::NGetTime
