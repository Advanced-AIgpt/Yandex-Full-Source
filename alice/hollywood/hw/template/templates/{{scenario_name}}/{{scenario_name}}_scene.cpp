#include "{{scenario_name}}_scene.h"

#include <alice/hollywood/library/scenarios/{{scenario_name}}/proto/{{scenario_name}}.pb.h>

namespace NAlice::NHollywoodFw::N{{ScenarioName}} {

T{{ScenarioName}}Scene::T{{ScenarioName}}Scene(const TScenario* owner)
    : TScene(owner, SCENE_NAME_DEFAULT)
{
    RegisterRenderer(&T{{ScenarioName}}Scene::Render);
}

TRetMain T{{ScenarioName}}Scene::Main(
        const T{{ScenarioName}}SceneArgs& args,
        const TRunRequest& runRequest,
        TStorage& storage,
        const TSource&) const
{
    T{{ScenarioName}}State state;
    state.SetAge(args.GetAge());
    if (!args.GetName().Empty()) {
        state.MutableName()->SetStringValue(args.GetName());
    }
    storage.SetScenarioState(state);

    T{{ScenarioName}}RenderArgs renderArgs;
    LOG_INFO(runRequest.Debug().Logger()) << "{{scenario_name}} scenario: winner";
    return TReturnValueRender(&T{{ScenarioName}}Scene::Render, renderArgs);
}

TRetResponse T{{ScenarioName}}Scene::Render(
        const T{{ScenarioName}}RenderArgs& args,
        TRender& render)
{
    render.CreateFromNlg("{{scenario_name}}", "render_result", args);
    return TReturnValueSuccess();
}

}  // namespace NAlice::NHollywood::N{{ScenarioName}}
