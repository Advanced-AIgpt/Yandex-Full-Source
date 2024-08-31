#include "route_manager_scene_show.h"

#include <alice/hollywood/library/framework/core/return_types.h>
#include <alice/hollywood/library/request/experiments.h>

#include <alice/protos/endpoint/capabilities/route_manager/route_manager.pb.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywoodFw::NRouteManager {

TRouteManagerSceneShow::TRouteManagerSceneShow(const TScenario* owner)
    : TScene(owner, SCENE_NAME_SHOW)
{
    RegisterRenderer(&TRouteManagerSceneShow::RenderShow);
}

TRetMain TRouteManagerSceneShow::Main(const TRouteManagerSceneArgsShow& args,
                              const TRunRequest& runRequest,
                              TStorage& storage,
                              const TSource& source) const
{
    Y_UNUSED(args);
    Y_UNUSED(storage);
    Y_UNUSED(source);

    auto deviceState = runRequest.Client().TryGetMessage<TDeviceState>();
    Y_ENSURE(deviceState, "Unable to get TDeviceState");
    const bool handleDeviceState = runRequest.Flags().IsExperimentEnabled(NAlice::NHollywood::EXP_HW_ROUTE_MANAGER_HANDLE_STATE);
    const bool frameExpected = deviceState->GetRouteManagerState().GetLayout() == TRouteManagerCapability::TState::Ride ||
                               deviceState->GetRouteManagerState().GetLayout() == TRouteManagerCapability::TState::UnknownLayout;
    TRouteManagerRenderArgsShow state;
    *state.MutableRouteManagerState() = deviceState->GetRouteManagerState();
    state.SetExpected(!handleDeviceState || frameExpected);

    return TReturnValueRender(&TRouteManagerSceneShow::RenderShow, state);
}

TRetResponse TRouteManagerSceneShow::RenderShow(const TRouteManagerRenderArgsShow& state, TRender& render) {
    auto resultState = state.GetRouteManagerState();
    if (state.GetExpected()) {
        render.CreateFromNlg(ROUTE_MANAGER_NLG, NLG_SHOW, state);

        NScenarios::TDirective directive;
        directive.MutableRouteManagerShowDirective();
        *render.GetResponseBody().MutableLayout()->AddDirectives() = std::move(directive);
        resultState.SetLayout(TRouteManagerCapability::TState::Map);
    } else {
        render.CreateFromNlg(ROUTE_MANAGER_NLG, NLG_ALREADY_SHOWN, state);
    }

    AddSuggests(render, resultState);
    return TReturnValueSuccess();
}

} // namespace NAlice::NHollywoodFw::NRouteManager
