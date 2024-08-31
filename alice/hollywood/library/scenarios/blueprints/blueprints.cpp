#include "blueprints.h"
#include "blueprints_scene.h"
#include "blueprints_fastdata.h"

#include <alice/hollywood/library/scenarios/blueprints/nlg/register.h>
#include <alice/library/analytics/common/product_scenarios.h>

namespace NAlice::NHollywoodFw::NBlueprints {

HW_REGISTER(TBlueprintsScenario);

/*
    Blueprints CTOR
*/
TBlueprintsScenario::TBlueprintsScenario()
    : TScenario(NProductScenarios::BLUEPRINTS)
{
    Register(&TBlueprintsScenario::Dispatch);
    RegisterScene<TBlueprintsScene>([this]() {
        RegisterSceneFn(&TBlueprintsScene::Main);
    });
    RegisterRenderer(&TBlueprintsScenario::RenderIrrelevant);

    // Additional functions
    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NBlueprints::NNlg::RegisterAll);
    AddFastData<TBlueprintsFastDataProto, TBlueprintsFastData>("blueprints/blueprints.pb");
}

/*
    Blueprints Dispatcher
    Check incoming data and find script which can execute request
*/
TRetScene TBlueprintsScenario::Dispatch(
    const TRunRequest& runRequest,
    const TStorage& storage,
    const TSource&) const
{
    // Load fastdata and compare all existing blueprints with input request
    const auto& fd = runRequest.System().GetFastData().GetFastData<TBlueprintsFastData>();
    const auto result = fd->Match(runRequest, storage);

    if (!result) {
        LOG_INFO(runRequest.Debug().Logger()) << "Can't find any blueprints to match input request, return irrelevant";
        return TReturnValueRenderIrrelevant(&TBlueprintsScenario::RenderIrrelevant, {});
    }
    return TReturnValueScene<TBlueprintsScene>(*result, result->GetIntentName());
}

/*
    Blueprints irrelevant renderer
*/
TRetResponse TBlueprintsScenario::RenderIrrelevant(
        const TBlueprintsRenderIrrelevant&,
        TRender& render)
{
    render.CreateFromNlg("blueprints", "error", NJson::TJsonValue{});
    return TReturnValueSuccess();
}

}  // namespace NAlice::NHollywood::NBlueprints
