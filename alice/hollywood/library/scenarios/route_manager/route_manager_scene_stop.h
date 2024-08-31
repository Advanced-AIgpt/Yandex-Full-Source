#pragma once

#include "route_manager_common.h"

#include <alice/hollywood/library/scenarios/route_manager/proto/route_manager_scenario_state.pb.h>

namespace NAlice::NHollywoodFw::NRouteManager {

inline constexpr TStringBuf SCENE_NAME_STOP = "scene_stop";
inline constexpr TStringBuf FRAME_NAME_STOP = "alice.route_manager.stop";
inline constexpr TStringBuf NLG_STOP = "stop";
inline constexpr TStringBuf NLG_CANT_STOP = "stop_cant_stop";

struct TFrameStop : public TFrame {
    TFrameStop(const TRequest::TInput& input)
        : TFrame(input, FRAME_NAME_STOP)
    {
    }
};

class TRouteManagerSceneStop : public TScene<TRouteManagerSceneArgsStop> {
public:
    TRouteManagerSceneStop(const TScenario* owner);
    TRetMain Main(const TRouteManagerSceneArgsStop& args,
                  const TRunRequest& runRequest,
                  TStorage& storage,
                  const TSource& source) const override;

    static TRetResponse RenderStop(const TRouteManagerRenderArgsStop&, TRender&);
};

} // namespace NAlice::NHollywoodFw::NRouteManager
