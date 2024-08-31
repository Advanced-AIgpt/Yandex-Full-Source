#include "modifier_request_factory.h"

#include "utils.h"

#include <alice/library/experiments/experiments.h>
#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/worldwide/language/is_alice_worldwide_language.h>

#include <alice/megamind/protos/modifiers/modifier_body.pb.h>

namespace NAlice::NMegamind::NModifiers {

// TAppHostModifierFactory ------------------------------------------------------------------------
TAppHostModifierRequestFactory::TAppHostModifierRequestFactory(TItemProxyAdapter& itemAdapter, const IContext& ctx, const TScenarioConfigRegistry& scenarioConfigRegistry)
    : ItemAdapter{itemAdapter}
    , Ctx{ctx}
    , ScenarioConfigRegistry{scenarioConfigRegistry} {
}

void TAppHostModifierRequestFactory::SetupModifierRequest(const TRequest& request,
                                                          const NScenarios::TScenarioResponseBody& responseBody,
                                                          const TString& scenarioName) {
    const auto& scenarioConfig = ScenarioConfigRegistry.GetScenarioConfig(scenarioName);
    const auto modifierRequest = NModifiers::ConstructModifierRequest(responseBody, Ctx, request, scenarioConfig);
    ItemAdapter.PutIntoContext(modifierRequest, NMegamind::AH_ITEM_MODIFIER_REQUEST);
}

TStatus TAppHostModifierRequestFactory::ApplyModifierResponse(TScenarioResponse& scenarioResponse, TMegamindAnalyticsInfoBuilder& analyticsInfo) {
    auto* responseBody = scenarioResponse.ResponseBodyIfExists();
    if (!responseBody) {
        LOG_ERR(Ctx.Logger()) << "No response body to apply modifier to";
        return Success();
    }

    TModifierResponse modifierResponse;
    if (const auto error = ItemAdapter.GetFromContext<TModifierResponse>(NMegamind::AH_ITEM_MODIFIER_RESPONSE).MoveTo(modifierResponse)) {
        LOG_ERR(Ctx.Logger()) << "Failed to request modifier: " << *error;
        return IsAliceWorldWideLanguage(Ctx.Language()) ? *error : Success();
    }

    LOG_DEBUG(Ctx.Logger()) << TLogMessageTag("Modifier response") << "Got response from modifiers: " << modifierResponse;

    *responseBody->MutableLayout() = std::move(*modifierResponse.MutableModifierBody()->MutableLayout());
    analyticsInfo.SetModifierAnalyticsInfo(modifierResponse.GetAnalyticsInfo());
    analyticsInfo.SetModifiersAnalyticsInfo(modifierResponse.GetAnalytics());

    LOG_INFO(Ctx.Logger()) << "Successfully applied modifier";
    return Success();
}

} // namespace NAlice::NMegamind::NModifiers
