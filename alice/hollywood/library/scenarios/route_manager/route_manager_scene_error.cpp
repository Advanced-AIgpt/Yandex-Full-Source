#include "route_manager_scene_error.h"

#include <alice/hollywood/library/framework/core/return_types.h>

#include <alice/protos/endpoint/capabilities/route_manager/route_manager.pb.h>

namespace NAlice::NHollywoodFw::NRouteManager {

TRouteManagerSceneError::TRouteManagerSceneError(const TScenario* owner)
    : TScene(owner, SCENE_NAME_ERROR)
{
    RegisterRenderer(&TRouteManagerSceneError::RenderError);
}

TRetMain TRouteManagerSceneError::Main(const TRouteManagerSceneArgsError& args,
                              const TRunRequest& runRequest,
                              TStorage& storage,
                              const TSource& source) const
{
    Y_UNUSED(args);
    Y_UNUSED(runRequest);
    Y_UNUSED(storage);
    Y_UNUSED(source);

    TRouteManagerRenderArgsError state;
    return TReturnValueRender(&TRouteManagerSceneError::RenderError, state);
}

TRetResponse TRouteManagerSceneError::RenderError(const TRouteManagerRenderArgsError& state, TRender& render) {
    render.CreateFromNlg(ROUTE_MANAGER_NLG, NLG_DEVICE_ERROR, state);
    AddSuggests(render, state.GetRouteManagerState());
    return TReturnValueSuccess();
}

} // namespace NAlice::NHollywoodFw::NRouteManager
