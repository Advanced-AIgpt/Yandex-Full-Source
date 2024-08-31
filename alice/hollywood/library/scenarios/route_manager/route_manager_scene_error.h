#pragma once

#include "route_manager_common.h"

#include <alice/hollywood/library/scenarios/route_manager/proto/route_manager_scenario_state.pb.h>

namespace NAlice::NHollywoodFw::NRouteManager {

inline constexpr TStringBuf SCENE_NAME_ERROR = "scene_error";
inline constexpr TStringBuf NLG_DEVICE_ERROR = "device_error";

class TRouteManagerSceneError : public TScene<TRouteManagerSceneArgsError> {
public:
    TRouteManagerSceneError(const TScenario* owner);
    TRetMain Main(const TRouteManagerSceneArgsError& args,
                  const TRunRequest& runRequest,
                  TStorage& storage,
                  const TSource& source) const override;

    static TRetResponse RenderError(const TRouteManagerRenderArgsError&, TRender&);
};

} // namespace NAlice::NHollywoodFw::NRouteManager
