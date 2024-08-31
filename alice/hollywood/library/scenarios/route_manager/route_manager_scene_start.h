#pragma once

#include "route_manager_common.h"

#include <alice/hollywood/library/scenarios/route_manager/proto/route_manager_scenario_state.pb.h>

namespace NAlice::NHollywoodFw::NRouteManager {

inline constexpr TStringBuf SCENE_NAME_START = "scene_start";
inline constexpr TStringBuf FRAME_NAME_START = "alice.route_manager.start";
inline constexpr TStringBuf NLG_START = "start";
inline constexpr TStringBuf NLG_CANT_START = "start_cant_start";

struct TFrameStart : public TFrame {
    TFrameStart(const TRequest::TInput& input)
        : TFrame(input, FRAME_NAME_START)
    {
    }
};

class TRouteManagerSceneStart : public TScene<TRouteManagerSceneArgsStart> {
public:
    TRouteManagerSceneStart(const TScenario* owner);
    TRetMain Main(const TRouteManagerSceneArgsStart& args,
                  const TRunRequest& runRequest,
                  TStorage& storage,
                  const TSource& source) const override;

    static TRetResponse RenderStart(const TRouteManagerRenderArgsStart&, TRender&);
};

} // namespace NAlice::NHollywoodFw::NRouteManager
