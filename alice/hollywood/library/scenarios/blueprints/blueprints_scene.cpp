#include "blueprints_scene.h"

#include <alice/hollywood/library/scenarios/blueprints/proto/blueprints.pb.h>

namespace NAlice::NHollywoodFw::NBlueprints {

/*
    Main Blueprints scene
    Process all scripts found in Dispatch() call
*/
TBlueprintsScene::TBlueprintsScene(const TScenario* owner)
    : TScene(owner, SCENE_NAME_DEFAULT)
{
    RegisterRenderer(&TBlueprintsScene::Render);
}

TRetMain TBlueprintsScene::Main(
        const TBlueprintsArgs& args,
        const TRunRequest& runRequest,
        TStorage& storage,
        const TSource& source) const
{
    Y_UNUSED(runRequest);
    Y_UNUSED(storage);
    Y_UNUSED(source);
    // Note blueprint scene doesn't have a login and just pass all arguments to Render function for final execution
    return TReturnValueRender(&TBlueprintsScene::Render, args);
}

TRetResponse TBlueprintsScene::Render(
        const TBlueprintsArgs& args,
        TRender& render)
{
    // TODO [DD] This function will be implemented later
    Y_UNUSED(args);
    Y_UNUSED(render);
    return TReturnValueSuccess();
}

}  // namespace NAlice::NHollywood::NBlueprints
