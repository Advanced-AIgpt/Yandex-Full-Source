#include "conjugator_modifier.h"
#include "layout_inspector.h"

#include <alice/megamind/protos/analytics/modifiers/conjugator/conjugator.pb.h>
#include <alice/megamind/protos/modifiers/modifier_body.pb.h>

#include <alice/protos/api/conjugator/api.pb.h>
#include <alice/protos/data/contextual_data.pb.h>

#include <util/stream/file.h>

namespace NAlice::NHollywood::NModifiers {

namespace {

const TString MODIFIER_TYPE = "conjugator";

ELang ExtractResponseLanguage(const TModifierFeatures& features) {
    const auto responseFeatureLanguage = features.GetContextualData().GetResponseLanguage();
    return (responseFeatureLanguage != ::NAlice::ELang::L_UNK ? responseFeatureLanguage : features.GetScenarioLanguage());
}

TMaybe<TStringBuf> TryFindConjugatorRequestItemName(const ELang language) {
    switch(language) {
    case ELang::L_ARA:
        return "ar_conjugator_request";
    default:
        return Nothing();
    }
}

NConjugator::TConjugateRequest BuildConjugateRequest(const ELang language, const TModifierBody& requestBody) {
    NConjugator::TConjugateRequest result;
    result.SetLanguage(language);

    auto uniquePhrases = THashSet<TStringBuf>(requestBody.GetLayout().CardsSize() + 1);
    const auto inspector = TLayoutInspectorForConjugatablePhrases(requestBody.GetLayout());
    inspector.InspectOutputSpeech([&result, &uniquePhrases](TOutputSpeechInfo outputSpeechInfo){
        if (uniquePhrases.insert(outputSpeechInfo.SimplePhrase).second) {
            *result.AddUnconjugatedPhrases() = outputSpeechInfo.SimplePhrase;
        }
    });
    inspector.InspectCards([&result, &uniquePhrases](TCardInfo cardInfo){
        if (uniquePhrases.insert(cardInfo.CardPhrase).second) {
            *result.AddUnconjugatedPhrases() = cardInfo.CardPhrase;
        }
    });
    return result;
}

THashMap<TStringBuf, const TString*> BuildConjugationsMap(
    const NConjugator::TConjugateRequest& conjugatorRequest, const NConjugator::TConjugateResponse& conjugatorResponse)
{
    Y_ENSURE(conjugatorRequest.UnconjugatedPhrasesSize() == conjugatorResponse.ConjugatedPhrasesSize(),
        "Uncojugated phrases count " << conjugatorRequest.UnconjugatedPhrasesSize() << " is not equal to " <<
        "conjugated phrases count " << conjugatorResponse.ConjugatedPhrasesSize());

    auto result = THashMap<TStringBuf, const TString*>(conjugatorRequest.UnconjugatedPhrasesSize());
    for(size_t i = 0; i < conjugatorRequest.UnconjugatedPhrasesSize(); ++i) {
        result.emplace(conjugatorRequest.GetUnconjugatedPhrases(i), &conjugatorResponse.GetConjugatedPhrases(i));
    }
    return result;
}

void ApplyConjugatorResponse(
    TResponseBodyBuilder& responseBody,
    ::NAlice::NModifiers::NConjugator::TConjugator& conjugatorAnalyticsInfo,
    const NConjugator::TConjugateRequest& conjugatorRequest,
    const NConjugator::TConjugateResponse& conjugatorResponse,
    TRTLogger& logger,
    const bool dumpDetailedAnalyticsInfo)
{
    const auto conjugationsMap = BuildConjugationsMap(conjugatorRequest, conjugatorResponse);
    conjugatorAnalyticsInfo.SetConjugatedPhrasesCount(conjugationsMap.size());

    const auto inspector = TLayoutInspectorForConjugatablePhrases(responseBody.GetModifierBody().GetLayout());
    inspector.InspectOutputSpeech([&](TOutputSpeechInfo outputSpeechInfo) {
        auto* conjugationPhrasesPair = dumpDetailedAnalyticsInfo ? conjugatorAnalyticsInfo.MutableOutputSpeechConjugationPhrasesPair() : nullptr;
        if (conjugationPhrasesPair) {
            conjugationPhrasesPair->SetUnconjugatedPhrase(TProtoStringType(outputSpeechInfo.SimplePhrase));
        }
        if (const auto* conjugatedPhrase = conjugationsMap.Value(outputSpeechInfo.SimplePhrase, nullptr)) {
            conjugatorAnalyticsInfo.SetIsOutputSpeechConjugated(true);
            if (conjugationPhrasesPair) {
                conjugationPhrasesPair->SetConjugatedPhrase(TProtoStringType(*conjugatedPhrase));
            }

            responseBody.SetVoice(outputSpeechInfo.StartingSpeakerTag + *conjugatedPhrase);
        } else {
            LOG_WARN(logger) << "Failed to find conjugated phrase for output speech: '" << outputSpeechInfo.SimplePhrase << "'";
        }
    });

    inspector.InspectCards([&](TCardInfo cardInfo) {
        auto* conjugationPhrasesPair = dumpDetailedAnalyticsInfo ? conjugatorAnalyticsInfo.AddCardConjugationPhrasesPairs() : nullptr;
        if (conjugationPhrasesPair) {
            conjugationPhrasesPair->SetUnconjugatedPhrase(TProtoStringType(cardInfo.CardPhrase));
        }
        if (const auto* conjugatedPhrase = conjugationsMap.Value(cardInfo.CardPhrase, nullptr)) {
            conjugatorAnalyticsInfo.SetConjugatedCardsCount(conjugatorAnalyticsInfo.GetConjugatedCardsCount() + 1);
            if (conjugationPhrasesPair) {
                conjugationPhrasesPair->SetConjugatedPhrase(TProtoStringType(*conjugatedPhrase));
            }

            auto card = cardInfo.Card;
            TLayoutInspectorForConjugatablePhrases::SetPhraseInCard(card, *conjugatedPhrase);
            responseBody.SetCard(std::move(card), cardInfo.Index);
        } else {
            LOG_WARN(logger) << "Failed to find conjugated phrase for card " << cardInfo.Index << ": '" << cardInfo.CardPhrase << "'";
        }
    });
}

} // namespace

TConjugatorModifier::TConjugatorModifier()
    : TBaseModifier(MODIFIER_TYPE)
{
}

void TConjugatorModifier::LoadResourcesFromPath(const TFsPath& modifierResourcesBasePath) {
    TFileInput configInput(modifierResourcesBasePath / "conjugatable_scenarios.pb.txt");
    const auto conjugatableScenariosConfig = ParseProtoText<TConjugatableScenariosConfig>(configInput.ReadAll());
    Configure(conjugatableScenariosConfig);
}

void TConjugatorModifier::Configure(const TConjugatableScenariosConfig& conjugatableScenariosConfig) {
    Y_ENSURE(!ConjugatableScenariosMatcher_, "Matcher is already initialized");
    ConjugatableScenariosMatcher_ = std::make_unique<TConjugatableScenariosMatcher>(conjugatableScenariosConfig);
}

void TConjugatorModifier::Prepare(TModifierPrepareContext prepareCtx) const {
    auto& logger = prepareCtx.ModifierContext.Logger();

    const auto language = ExtractResponseLanguage(prepareCtx.ModifierContext.GetFeatures());
    const auto requestItemName = TryFindConjugatorRequestItemName(language);
    if (!requestItemName.Defined()) {
        LOG_DEBUG(logger) << "Language " << ELang_Name(language) << " is not conjugatable";
        return;
    }

    const auto responseConjugationStatus = prepareCtx.ModifierContext.GetFeatures().GetContextualData().GetConjugator().GetResponseConjugationStatus();
    if (responseConjugationStatus != NData::TContextualData_TConjugator_EResponseConjugationStatus_Undefined) {
        if (responseConjugationStatus == NData::TContextualData_TConjugator_EResponseConjugationStatus_Conjugated)
        {
            LOG_DEBUG(logger) << "Scenario response is already conjugated";
            return;
        }
    } else {
        const auto productScenarioName = prepareCtx.ModifierContext.GetFeatures().GetProductScenarioName();
        Y_ENSURE(ConjugatableScenariosMatcher_, "Matcher is not initialized");
        if (!ConjugatableScenariosMatcher_->IsConjugatableLanguageScenario(language, productScenarioName, prepareCtx.ModifierContext.ExpFlags())) {
            LOG_DEBUG(logger) << "Request is considered as not conjubatable for language " << ELang_Name(language) <<
                " and product scenario name = " << productScenarioName;
            return;
        }
    }

    const auto conjugateRequest = BuildConjugateRequest(language, prepareCtx.RequestBody);

    LOG_INFO(logger) << "Prepared conjugator request with " << conjugateRequest.UnconjugatedPhrasesSize() <<
        " phrases and language = " << ELang_Name(language);
    LOG_DEBUG(logger) << "Conjugate request: " << conjugateRequest;

    if (!conjugateRequest.GetUnconjugatedPhrases().empty()) {
        prepareCtx.ExternalSourcesRequestCollector.AddRequest(conjugateRequest, requestItemName.GetRef());
    }
}

TApplyResult TConjugatorModifier::TryApply(TModifierApplyContext applyCtx) const {
    auto& logger = applyCtx.ModifierContext.Logger();

    const auto conjugatorRequest = applyCtx.ExternalSourcesResponseRetriever
        .TryGetResponse<NConjugator::TConjugateRequest>(CONJUGATOR_REQUEST_ITEM_NAME);

    if (conjugatorRequest.Empty()) {
        LOG_DEBUG(logger) << "Not found conjugator request item: " << CONJUGATOR_REQUEST_ITEM_NAME;
        return TNonApply(TNonApply::EType::NotApplicable);
    }

    const auto conjugatorResponse = applyCtx.ExternalSourcesResponseRetriever
        .TryGetResponse<NConjugator::TConjugateResponse>(CONJUGATOR_RESPONSE_ITEM_NAME);

    Y_ENSURE(conjugatorResponse.Defined(), "Not found conjugator response item: " << CONJUGATOR_RESPONSE_ITEM_NAME);

    LOG_INFO(logger) << "Applying conjugator response with " << conjugatorResponse->ConjugatedPhrasesSize() << " phrases";
    LOG_DEBUG(logger) << "Conjugator response: " << conjugatorResponse.GetRef();

    ::NAlice::NModifiers::NConjugator::TConjugator conjugatorAnalyticsInfo;
    ApplyConjugatorResponse(
        applyCtx.ResponseBody, conjugatorAnalyticsInfo,
        conjugatorRequest.GetRef(), conjugatorResponse.GetRef(),
        logger, applyCtx.ModifierContext.HasExpFlag(EXP_CONJUGATOR_MODIFIER_DUMP_DETAILED_ANALYTICS_INFO));

    applyCtx.AnalyticsInfo.SetConjugator(std::move(conjugatorAnalyticsInfo));

    return Nothing();
}

} // namespace NAlice::NHollywood::NModifiers
