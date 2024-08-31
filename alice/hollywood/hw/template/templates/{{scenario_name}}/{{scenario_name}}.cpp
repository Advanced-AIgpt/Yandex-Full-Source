#include "{{scenario_name}}.h"
#include "{{scenario_name}}_scene.h"

#include <alice/hollywood/library/scenarios/{{scenario_name}}/nlg/register.h>
#include <alice/library/analytics/common/product_scenarios.h>

namespace NAlice::NHollywoodFw::N{{ScenarioName}} {

HW_REGISTER(T{{ScenarioName}}Scenario);

T{{ScenarioName}}Scenario::T{{ScenarioName}}Scenario()
    : TScenario(NProductScenarios::{{SCENARIO_NAME}})
{
    Register(&T{{ScenarioName}}Scenario::Dispatch);
    RegisterScene<T{{ScenarioName}}Scene>([this]() {
        RegisterSceneFn(&T{{ScenarioName}}Scene::Main);
    });

    RegisterRenderer(&T{{ScenarioName}}Scenario::RenderIrrelevant);

    // Additional functions
    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::N{{ScenarioName}}::NNlg::RegisterAll);
}

TRetScene T{{ScenarioName}}Scenario::Dispatch(
        const TRunRequest& runRequest,
        const TStorage& storage,
        const TSource&) const
{
    const T{{ScenarioName}}Frame frame(runRequest.Input());

    if (!frame.Defined()) {
        LOG_ERR(runRequest.Debug().Logger()) << "Semantic frames not found";
        return TReturnValueRenderIrrelevant(&T{{ScenarioName}}Scenario::RenderIrrelevant, {});
    }

    T{{ScenarioName}}SceneArgs args;
    // TODO: scenario logic
    args.SetAge(frame.Age.Value);
    if (frame.Nominal.Defined()) {
        args.SetName(*frame.Nominal.Value);
    }

    Y_UNUSED(storage);
    return TReturnValueScene<T{{ScenarioName}}Scene>(args, frame.GetName());
}

TRetResponse T{{ScenarioName}}Scenario::RenderIrrelevant(
        const T{{ScenarioName}}RenderIrrelevant&,
        TRender& render)
{
    render.CreateFromNlg("{{scenario_name}}", "error", NJson::TJsonValue{});
    return TReturnValueSuccess();
}

}  // namespace NAlice::NHollywood::N{{ScenarioName}}
