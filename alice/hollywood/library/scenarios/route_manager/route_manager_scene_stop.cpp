#include "route_manager_scene_stop.h"

#include <alice/hollywood/library/framework/core/return_types.h>
#include <alice/hollywood/library/request/experiments.h>

#include <alice/protos/endpoint/capabilities/route_manager/route_manager.pb.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywoodFw::NRouteManager {

TRouteManagerSceneStop::TRouteManagerSceneStop(const TScenario* owner)
    : TScene(owner, SCENE_NAME_STOP)
{
    RegisterRenderer(&TRouteManagerSceneStop::RenderStop);
}

TRetMain TRouteManagerSceneStop::Main(const TRouteManagerSceneArgsStop& args,
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
    const bool frameExpected = deviceState->GetRouteManagerState().GetRoute() == TRouteManagerCapability::TState::Moving;
    TRouteManagerRenderArgsStop state;
    *state.MutableRouteManagerState() = deviceState->GetRouteManagerState();
    state.SetExpected(!handleDeviceState || frameExpected);

    return TReturnValueRender(&TRouteManagerSceneStop::RenderStop, state);
}

TRetResponse TRouteManagerSceneStop::RenderStop(const TRouteManagerRenderArgsStop& state, TRender& render) {
    auto resultState = state.GetRouteManagerState();
    if (state.GetExpected()) {
        render.CreateFromNlg(ROUTE_MANAGER_NLG, NLG_STOP, state);

        NScenarios::TDirective directive;
        directive.MutableRouteManagerStopDirective();
        *render.GetResponseBody().MutableLayout()->AddDirectives() = std::move(directive);
        resultState.SetRoute(TRouteManagerCapability::TState::Stopping);
    } else {
        render.CreateFromNlg(ROUTE_MANAGER_NLG, NLG_CANT_STOP, state);
    }

    AddSuggests(render, resultState);
    return TReturnValueSuccess();
}

} // namespace NAlice::NHollywoodFw::NRouteManager