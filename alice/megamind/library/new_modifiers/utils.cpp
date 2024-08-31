#include "utils.h"

#include <alice/megamind/library/scenarios/helpers/get_request_language/get_scenario_request_language.h>
#include <alice/megamind/protos/modifiers/modifier_body.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/protos/data/contextual_data.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <alice/library/experiments/utils.h>

namespace NAlice::NMegamind::NModifiers {

TModifierRequest ConstructModifierRequest(const NScenarios::TScenarioResponseBody& responseBody, const IContext& ctx,
                                          const TRequest& request, const TScenarioConfig& scenarioConfig) {
    const auto& skr = ctx.SpeechKitRequest();
    TModifierRequest modifierRequest;
    *modifierRequest.MutableModifierBody()->MutableLayout() = responseBody.GetLayout();
    {
        auto* features = modifierRequest.MutableFeatures();
        if (responseBody.HasAnalyticsInfo()) {
            const auto& scenarioAnalyticsInfo = responseBody.GetAnalyticsInfo();
            features->SetProductScenarioName(scenarioAnalyticsInfo.GetProductScenarioName());
            features->SetIntent(scenarioAnalyticsInfo.GetIntent());
        }
        if (responseBody.HasContextualData()) {
            *features->MutableContextualData() = responseBody.GetContextualData();
        }

        features->SetPromoType(request.GetOptions().GetPromoType());

        {
            auto* soundSettings = features->MutableSoundSettings();
            if (const auto& whisperInfo = request.GetWhisperInfo(); whisperInfo.Defined()) {
                soundSettings->SetIsWhisper(whisperInfo->IsWhisper());
                const auto& ownerScenario = request.GetCallbackOwnerScenario().GetOrElse(scenarioConfig.GetName());
                soundSettings->SetIsWhisperTagDisabled(ctx.ScenarioConfig(ownerScenario).GetDisableWhisperAsCallbackOwner());
                soundSettings->SetIsPreviousRequestWhisper(whisperInfo->IsPreviousRequestWhisper());
            }
            const auto& deviceState = skr->GetRequest().GetDeviceState();
            if (deviceState.HasSoundLevel()) {
                soundSettings->SetSoundLevel(deviceState.GetSoundLevel());
            }
            if (deviceState.GetMultiroom().HasMultiroomSessionId()) {
                soundSettings->SetMultiroomSessionId(deviceState.GetMultiroom().GetMultiroomSessionId());
            }
        }

        features->SetScenarioLanguage(static_cast<ELang>(GetScenarioRequestLanguage(scenarioConfig, ctx)));
    }

    {
        auto* baseRequest = modifierRequest.MutableBaseRequest();
        baseRequest->SetRandomSeed(skr.GetSeed());
        baseRequest->SetRequestId(skr->GetHeader().GetRequestId());
        baseRequest->SetServerTimeMs(request.GetServerTimeMs());
        *baseRequest->MutableInterfaces() = request.GetInterfaces();
        NMegamind::TExpFlagsToStructVisitor(*baseRequest->MutableExperiments())
            .Visit(skr->GetRequest().GetExperiments());
        baseRequest->SetUserLanguage(static_cast<ELang>(ctx.Language()));
    }
    return modifierRequest;
}

} // namespace NAlice::NMegamind::NModifiers
