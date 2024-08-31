#include "vins_dispatcher.h"
#include "vins_scene.h"

#include <alice/library/analytics/common/product_scenarios.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywoodFw::NVins {

namespace {

bool UseScreenDeviceRender(const TRunRequest& request) {
    return (!request.Flags().IsExperimentEnabled("show_route_div_card_centaur_off") &&
             request.Flags().IsExperimentEnabled("show_route_div_card_centaur_on") &&
             request.Client().GetInterfaces().GetSupportsShowView());
}

}

HW_REGISTER(TVinsScenario);

TVinsScenario::TVinsScenario()
    : TScenario(NProductScenarios::VINS)
{
    Register(&TVinsScenario::Dispatch);
    RegisterScene<TVinsScene>([this]() {
        RegisterSceneFn(&TVinsScene::MainSetup);
        RegisterSceneFn(&TVinsScene::Main);
        RegisterSceneFn(&TVinsScene::ApplySetup);
        RegisterSceneFn(&TVinsScene::Apply);
        RegisterSceneFn(&TVinsScene::CommitSetup);
        RegisterSceneFn(&TVinsScene::Commit);
    });

    SetApphostGraph(ScenarioRequest() >>
                    NodeRun("run_prepare") >>
                    NodeMain("run_parse") >>
                    ScenarioResponse());
    SetApphostGraph(ScenarioApply() >>
                    NodeApplySetup("apply_prepare") >>
                    NodeApply("apply_parse") >>
                    ScenarioResponse());
    SetApphostGraph(ScenarioCommit() >>
                    NodeCommitSetup("commit_prepare") >>
                    NodeCommit("commit_parse") >>
                    ScenarioResponse());
    EnableDebugGraph();

    SetDivRenderMode(EDivRenderMode::PrepareForOutsideMerge);
}

TRetScene TVinsScenario::Dispatch(const TRunRequest& request,
                                  const TStorage& storage,
                                  const TSource& source) const
{
    Y_UNUSED(storage);
    Y_UNUSED(source);

    TVinsSceneArgs args;
    args.SetUseVinsResponseProto(true);
    args.SetUseScreenDeviceRender(UseScreenDeviceRender(request));
    return TReturnValueScene<TVinsScene>(args);
}

void TVinsScenario::Hook(THookInputInfo& info,
                         TScenarioRunResponse& runResponse) const
{
    if (info.RenderArguments && info.RenderArguments.GetRef().Is<TScenarioRunResponse>()) {
        info.RenderArguments.GetRef().UnpackTo(&runResponse);
        if (runResponse.GetResponseCase() == TScenarioRunResponse::kCommitCandidate) {
            *runResponse.MutableCommitCandidate()->MutableArguments() = PrepareArguments(runResponse.GetCommitCandidate().GetArguments(), info.NewContext);
        }
    }
    if (info.RenderArguments && info.RenderArguments.GetRef().Is<TScreenDeviceRender>()) {
        TScreenDeviceRender renderArgs;
        info.RenderArguments.GetRef().UnpackTo(&renderArgs);
        renderArgs.MutableResponse()->MutableResponseBody()->MutableLayout()->MutableDirectives()->Swap(
            runResponse.MutableResponseBody()->MutableLayout()->MutableDirectives()
        );
        runResponse.Swap(renderArgs.MutableResponse());
        if (runResponse.GetResponseCase() == TScenarioRunResponse::kCommitCandidate) {
            *runResponse.MutableCommitCandidate()->MutableArguments() = PrepareArguments(runResponse.GetCommitCandidate().GetArguments(), info.NewContext);
        }
    }
}

void TVinsScenario::Hook(THookInputInfo& info,
                         TScenarioApplyResponse& applyResponse) const
{
    if (info.RenderArguments && info.RenderArguments.GetRef().Is<TScenarioApplyResponse>()) {
        info.RenderArguments.GetRef().UnpackTo(&applyResponse);
    }
}

} // namespace NAlice::NHollywoodFw::NVins
