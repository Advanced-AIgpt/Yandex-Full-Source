#include "route_manager.h"

#include "route_manager_scene_continue.h"
#include "route_manager_scene_error.h"
#include "route_manager_scene_show.h"
#include "route_manager_scene_start.h"
#include "route_manager_scene_stop.h"

#include <alice/hollywood/library/scenarios/route_manager/nlg/register.h>
#include <alice/hollywood/library/scenarios/route_manager/proto/route_manager_scenario_state.pb.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <alice/megamind/protos/common/device_state.pb.h>

namespace NAlice::NHollywoodFw::NRouteManager {

#define TryHandleFrame(frameName) \
    if (frame##frameName.Defined()) \
        return TReturnValueScene<TRouteManagerScene##frameName>( \
            TRouteManagerSceneArgs##frameName{}, \
            frame##frameName.GetName() \
        );

HW_REGISTER(TRouteManagerScenario);

TRouteManagerScenario::TRouteManagerScenario()
    : TScenario(NProductScenarios::ROUTE_MANAGER)
{
    Register(&TRouteManagerScenario::Dispatch);
    RegisterScene<TRouteManagerSceneContinue>([this]() {
        RegisterSceneFn(&TRouteManagerSceneContinue::Main);
    });
    RegisterScene<TRouteManagerSceneError>([this]() {
        RegisterSceneFn(&TRouteManagerSceneError::Main);
    });
    RegisterScene<TRouteManagerSceneShow>([this]() {
        RegisterSceneFn(&TRouteManagerSceneShow::Main);
    });
    RegisterScene<TRouteManagerSceneStart>([this]() {
        RegisterSceneFn(&TRouteManagerSceneStart::Main);
    });
    RegisterScene<TRouteManagerSceneStop>([this]() {
        RegisterSceneFn(&TRouteManagerSceneStop::Main);
    });

    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NRouteManager::NNlg::RegisterAll);
    SetProductScenarioName(NProductScenarios::ROUTE_MANAGER);
}

TRetScene TRouteManagerScenario::Dispatch(const TRunRequest& runRequest,
                                 const TStorage&,
                                 const TSource&) const
{
    auto deviceState = runRequest.Client().TryGetMessage<TDeviceState>();
    Y_ENSURE(deviceState, "Unable to get TDeviceState");
    auto& logger = runRequest.Debug().Logger();
    LOG_INFO(logger) << "RouteManagerState: " << deviceState->GetRouteManagerState();

    TFrameShow frameShow(runRequest.Input());
    TFrameStart frameStart(runRequest.Input());
    TFrameStop frameStop(runRequest.Input());
    TFrameContinue frameContinue(runRequest.Input());

    switch (deviceState->GetRouteManagerState().GetRoute()) {
        case TRouteManagerCapability::TState::Stopped:
            TryHandleFrame(Continue);
            TryHandleFrame(Show);
            TryHandleFrame(Stop);
            TryHandleFrame(Start);
            break;
        case TRouteManagerCapability::TState::Stopping:
            TryHandleFrame(Continue);
            TryHandleFrame(Show);
            TryHandleFrame(Stop);
            TryHandleFrame(Start);
            break;
        case TRouteManagerCapability::TState::Moving:
            TryHandleFrame(Stop);
            TryHandleFrame(Show);
            TryHandleFrame(Start);
            TryHandleFrame(Continue);
            break;
        case TRouteManagerCapability::TState::WaitingPassenger:
            TryHandleFrame(Start);
            TryHandleFrame(Show);
            TryHandleFrame(Stop);
            TryHandleFrame(Continue);
            break;
        case TRouteManagerCapability::TState::Finished:
            TryHandleFrame(Stop);
            TryHandleFrame(Show);
            TryHandleFrame(Start);
            TryHandleFrame(Continue);
            break;
        case TRouteManagerCapability::TState::UnknownRoute:
        case TRouteManagerCapability_TState_TRoute_TRouteManagerCapability_TState_TRoute_INT_MIN_SENTINEL_DO_NOT_USE_:
        case TRouteManagerCapability_TState_TRoute_TRouteManagerCapability_TState_TRoute_INT_MAX_SENTINEL_DO_NOT_USE_:
            LOG_WARNING(logger) << "Got irrelevant RouteManagerState";
            return TReturnValueScene<TRouteManagerSceneError>(TRouteManagerSceneArgsError{}, "");
    }

    return TReturnValueRenderIrrelevant("route_manager", "error");
}

} // namespace NAlice::NHollywoodFw::NRouteManager
