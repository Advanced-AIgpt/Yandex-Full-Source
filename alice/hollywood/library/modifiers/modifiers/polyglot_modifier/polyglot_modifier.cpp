#include "polyglot_modifier.h"

#include <alice/hollywood/library/modifiers/modifiers/polyglot_modifier/proto/polyglot_translation_request.pb.h>
#include <alice/hollywood/library/modifiers/modifiers/polyglot_modifier/output_speech_modifier.h>

#include <alice/library/apphost_request/request_builder.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/json/json.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/nlg/library/voice_prefix/voice_prefix.h>
#include <alice/protos/data/contextual_data.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/protobuf/util/walk.h>

#include <util/string/join.h>

namespace NAlice::NHollywood::NModifiers {

namespace {

const TString MODIFIER_TYPE = "polyglot";

TString TryGetPhraseToTranslate(const NProtoBuf::Message& message, const NProtoBuf::FieldDescriptor* field) {
    if (field->options().HasExtension(NAlice::NScenarios::ApplyPolyglot) &&
        field->options().GetExtension(NAlice::NScenarios::ApplyPolyglot))
    {
        if (field->type() == NProtoBuf::FieldDescriptor::Type::TYPE_STRING) {
            return message.GetReflection()->GetString(message, field);
        }
    }
    return TString();
}

ELanguage ExtractResponseLanguage(const TModifierFeatures& features) {
    const auto responseFeatureLanguage = features.GetContextualData().GetResponseLanguage();
    const auto finalLanguageProto =
        (responseFeatureLanguage != ::NAlice::ELang::L_UNK ? responseFeatureLanguage : features.GetScenarioLanguage());
    return static_cast<ELanguage>(finalLanguageProto);
}

TString BuildTranslationLanguagePair(IModifierContext& modifierContext) {
    auto& logger = modifierContext.Logger();

    if (modifierContext.HasExpFlag(EXP_DISABLE_RESPONSE_TRANSLATION)) {
        LOG_INFO(logger) << "Translation is disabled by experiment flag " << EXP_DISABLE_RESPONSE_TRANSLATION;
        return TString();
    }

    const auto expLanguagePair = GetExperimentValueWithPrefix(
        modifierContext.ExpFlags(), EXP_RESPONSE_TRANSLATION_PREFIX).GetOrElse(TStringBuf());
    if (expLanguagePair) {
        LOG_DEBUG(logger) << "Translation language is obtained from experimental flag " << EXP_RESPONSE_TRANSLATION_PREFIX;
        return TString(expLanguagePair);
    }

    const auto srcLang = ExtractResponseLanguage(modifierContext.GetFeatures());
    const auto dstLang = static_cast<ELanguage>(modifierContext.GetBaseRequest().GetUserLanguage());

    if (srcLang == dstLang) {
        LOG_INFO(logger) << "Translation is not required, response already has language " << IsoNameByLanguage(srcLang);
        return TString();
    }

    return Join('-', IsoNameByLanguage(srcLang), IsoNameByLanguage(dstLang));
}

TPolyglotTranslationRequest BuildPolyglotTranslationRequest(TRTLogger& logger, const TModifierBody& modifierBody) {
    auto result = TPolyglotTranslationRequest();
    THashSet<TString> uniquePhrases;

    NProtoBuf::WalkReflection(modifierBody, [&](const NProtoBuf::Message& message, const NProtoBuf::FieldDescriptor* field) {
        if (auto phrase = TryGetPhraseToTranslate(message, field)) {
            if (uniquePhrases.insert(phrase).second) {
                LOG_DEBUG(logger) << "Translating phrase: " << phrase;
                result.AddUntranslatedPhrases(std::move(phrase));
            }
        }
        return true;
    });
    return result;
}

NAppHostHttp::THttpRequest BuildTranslationHttpRequest(
    const TStringBuf translationLanguagePair,
    const TPolyglotTranslationRequest& polyglotTranslationRequest)
{
    auto httpRequestBuilder = NAppHostRequest::TAppHostHttpProxyRequestBuilder();
    httpRequestBuilder.AddCgiParam("srv", "alice");
    httpRequestBuilder.AddCgiParam("lang", translationLanguagePair);
    for (const auto& untranslatedPhrase : polyglotTranslationRequest.GetUntranslatedPhrases()) {
        httpRequestBuilder.AddCgiParam("text", untranslatedPhrase);
    }
    return httpRequestBuilder.CreateRequest();
}

THashMap<TStringBuf, TString> BuildTranslationsMap(
    TRTLogger& logger,
    const TPolyglotTranslationRequest& polyglotTranslationRequest,
    const NAppHostHttp::THttpResponse& polyglotHttpResponse)
{
    Y_ENSURE(polyglotHttpResponse.GetStatusCode() == HttpCodes::HTTP_OK,
        "Polyglot http response status code is not ok: " << polyglotHttpResponse.GetStatusCode());

    try {
        const auto json = JsonFromString(polyglotHttpResponse.GetContent());
        Y_ENSURE(!json.Has("error"), "Polyglot http response contains error: " << json["error"].GetString());

        const auto& translatedPhrases = json["text"].GetArraySafe();
        Y_ENSURE(polyglotTranslationRequest.UntranslatedPhrasesSize() == translatedPhrases.size(),
            "Untranslated phrases count " << polyglotTranslationRequest.UntranslatedPhrasesSize() <<
            " is not equal to translated phrases count " << translatedPhrases.size());

        auto result = THashMap<TStringBuf, TString>(translatedPhrases.size());
        for (size_t i = 0; i < translatedPhrases.size(); ++i) {
            const auto& untranslatedPhrase = polyglotTranslationRequest.GetUntranslatedPhrases(i);
            const auto& translatedPhrase = translatedPhrases[i].GetStringSafe();
            result.emplace(untranslatedPhrase, translatedPhrase);
        }
        return result;
    } catch (...) {
        LOG_ERR(logger) << "Failed to parse polyglot http response: " << polyglotHttpResponse;
        throw;
    }
}

size_t ApplyTranslations(TRTLogger& logger, const THashMap<TStringBuf, TString>& translationsMap, TResponseBodyBuilder& responseBody) {
    size_t appliedTranslationsCount = 0;
    NProtoBuf::WalkReflection(responseBody.MutableModifierBody(), [&](NProtoBuf::Message& message, const NProtoBuf::FieldDescriptor* field) {
        if (const auto untranslatedPhrase = TryGetPhraseToTranslate(message, field)) {
            if (const auto* translatedPhrase = translationsMap.FindPtr(untranslatedPhrase)) {
                LOG_DEBUG(logger) << "Translated '" << untranslatedPhrase << "' -> '" << *translatedPhrase << "'";
                message.GetReflection()->SetString(&message, field, *translatedPhrase);
                ++appliedTranslationsCount;
            } else {
                LOG_WARN(logger) << "Failed to translate " << untranslatedPhrase;
                message.GetReflection()->ClearField(&message, field);
            }
        }
        return true;
    });
    return appliedTranslationsCount;
}

bool ApplyVoicePrefix(IModifierContext& modifierContext, TResponseBodyBuilder& responseBody) {
    if (responseBody.GetModifierBody().GetLayout().GetOutputSpeech().empty()) {
        return false;
    }

    TString voicePrefix;

    const auto experimentFlagsVoice = GetExperimentValueWithPrefix(
        modifierContext.ExpFlags(), EXP_POLYGLOT_VOICE_PREFIX).GetOrElse(TStringBuf());
    const auto language = static_cast<ELanguage>(modifierContext.GetBaseRequest().GetUserLanguage());
    if (experimentFlagsVoice) {
        voicePrefix = Base64Decode(experimentFlagsVoice);
        LOG_DEBUG(modifierContext.Logger()) << "Using voice prefix from experimental flag: " << voicePrefix;
    } else if (const auto nlgVoicePrefix = NNlg::GetVoicePrefixForLanguage(language)) {
        voicePrefix = nlgVoicePrefix;
        LOG_DEBUG(modifierContext.Logger()) << "Using default NLG voice prefix: " << voicePrefix;
    }

    if (voicePrefix.empty()) {
        return false;
    }

    auto outputSpeech = responseBody.GetModifierBody().GetLayout().GetOutputSpeech();
    TOutputSpeechModifier(voicePrefix).ModifyOutputSpeech(outputSpeech);
    responseBody.SetVoice(outputSpeech);
    return true;
}

} // namespace

TPolyglotModifier::TPolyglotModifier()
    : TBaseModifier(MODIFIER_TYPE)
{
}

void TPolyglotModifier::Prepare(TModifierPrepareContext prepareCtx) const {
    auto& logger = prepareCtx.ModifierContext.Logger();

    const auto translationLanguagePair = BuildTranslationLanguagePair(prepareCtx.ModifierContext);
    if (translationLanguagePair.empty()) {
        return;
    }

    const auto polyglotTranslationRequest = BuildPolyglotTranslationRequest(logger, prepareCtx.RequestBody);
    LOG_INFO(logger) << "Preparing to translate " << polyglotTranslationRequest.UntranslatedPhrasesSize() <<
        " phrases with language pair = " << translationLanguagePair;
    if (polyglotTranslationRequest.GetUntranslatedPhrases().empty()) {
        return;
    }

    const auto polyglotHttpRequest = BuildTranslationHttpRequest(translationLanguagePair, polyglotTranslationRequest);

    prepareCtx.ExternalSourcesRequestCollector.AddRequest(polyglotTranslationRequest, AH_ITEM_POLYGLOT_REQUEST_NAME);
    prepareCtx.ExternalSourcesRequestCollector.AddRequest(polyglotHttpRequest, AH_ITEM_POLYGLOT_HTTP_REQUEST_NAME);
}

TApplyResult TPolyglotModifier::TryApply(TModifierApplyContext applyCtx) const {
    auto& logger = applyCtx.ModifierContext.Logger();

    const auto polyglotTranslationRequest = applyCtx.ExternalSourcesResponseRetriever
        .TryGetResponse<TPolyglotTranslationRequest>(AH_ITEM_POLYGLOT_REQUEST_NAME);

    if (polyglotTranslationRequest.Empty()) {
        LOG_DEBUG(logger) << "Not found polyglot translation request item: " << AH_ITEM_POLYGLOT_REQUEST_NAME;
        return TNonApply(TNonApply::EType::NotApplicable);
    }

    const auto polyglotHttpResponse = applyCtx.ExternalSourcesResponseRetriever
        .TryGetResponse<NAppHostHttp::THttpResponse>(AH_ITEM_POLYGLOT_HTTP_RESPONSE_NAME);

    Y_ENSURE(polyglotHttpResponse.Defined(), "Not found polyglot http response item: " << AH_ITEM_POLYGLOT_HTTP_RESPONSE_NAME);
    LOG_DEBUG(logger) << "Polyglot http response: " << polyglotHttpResponse.GetRef();

    const auto translationsMap = BuildTranslationsMap(logger, polyglotTranslationRequest.GetRef(), polyglotHttpResponse.GetRef());
    const auto appliedTranslationsCount = ApplyTranslations(logger, translationsMap, applyCtx.ResponseBody);
    const auto appliedVoicePrefix = ApplyVoicePrefix(applyCtx.ModifierContext, applyCtx.ResponseBody);
    applyCtx.AnalyticsInfo.SetPolyglot(appliedTranslationsCount, translationsMap.size());

    LOG_INFO(logger) << "Successfully translated " << appliedTranslationsCount <<
        " phrases with " << translationsMap.size() << " unique translations, and " <<
        (appliedVoicePrefix ? "" : "not ") << "added voice prefix";
    return Nothing();
}

} // namespace NAlice::NHollywood::NModifiers
