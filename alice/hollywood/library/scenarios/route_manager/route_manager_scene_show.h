#pragma once

#include "route_manager_common.h"

#include <alice/hollywood/library/scenarios/route_manager/proto/route_manager_scenario_state.pb.h>

namespace NAlice::NHollywoodFw::NRouteManager {

inline constexpr TStringBuf SCENE_NAME_SHOW = "scene_show";
inline constexpr TStringBuf FRAME_NAME_SHOW = "alice.route_manager.show";
inline constexpr TStringBuf NLG_SHOW = "show";
inline constexpr TStringBuf NLG_ALREADY_SHOWN = "show_already_shown";

struct TFrameShow : public TFrame {
    TFrameShow(const TRequest::TInput& input)
        : TFrame(input, FRAME_NAME_SHOW)
    {
    }
};

class TRouteManagerSceneShow : public TScene<TRouteManagerSceneArgsShow> {
public:
    TRouteManagerSceneShow(const TScenario* owner);
    TRetMain Main(const TRouteManagerSceneArgsShow& args,
                  const TRunRequest& runRequest,
                  TStorage& storage,
                  const TSource& source) const override;

    static TRetResponse RenderShow(const TRouteManagerRenderArgsShow&, TRender&);
};

} // namespace NAlice::NHollywoodFw::NRouteManager
