#pragma once

#include "route_manager_common.h"

#include <alice/hollywood/library/scenarios/route_manager/proto/route_manager_scenario_state.pb.h>

namespace NAlice::NHollywoodFw::NRouteManager {

inline constexpr TStringBuf SCENE_NAME_CONTINUE = "scene_continue";
inline constexpr TStringBuf FRAME_NAME_CONTINUE = "alice.route_manager.continue";
inline constexpr TStringBuf NLG_CONTINUE = "continue";
inline constexpr TStringBuf NLG_CANT_CONTINUE = "continue_cant_continue";

struct TFrameContinue : public TFrame {
    TFrameContinue(const TRequest::TInput& input)
        : TFrame(input, FRAME_NAME_CONTINUE)
    {
    }
};

class TRouteManagerSceneContinue : public TScene<TRouteManagerSceneArgsContinue> {
public:
    TRouteManagerSceneContinue(const TScenario* owner);
    TRetMain Main(const TRouteManagerSceneArgsContinue& args,
                  const TRunRequest& runRequest,
                  TStorage& storage,
                  const TSource& source) const override;

    static TRetResponse RenderContinue(const TRouteManagerRenderArgsContinue&, TRender&);
};

} // namespace NAlice::NHollywoodFw::NRouteManager
