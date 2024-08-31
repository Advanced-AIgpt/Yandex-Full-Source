#include "polyglot.h"

#include "components.h"

#include <alice/megamind/library/apphost_request/request_builder.h>
#include <alice/megamind/library/biometry/biometry.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/sources/request.h>

#include <alice/library/json/json.h>

#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NMegamind {
namespace {

TErrorOr<TString> GetTranslationFromContext(IAppHostCtx& ahCtx) {
    NAppHostHttp::THttpResponse responseProto;
    if (TStatus err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_POLYGLOT_HTTP_RESPONSE_NAME, responseProto)) {
        return std::move(*err);
    }

    if (responseProto.GetStatusCode() != HTTP_OK) {
        return TError{TError::EType::Critical} << "Translate API answer code is not HTTP_OK: "
                                               << responseProto.ShortUtf8DebugString();
    }

    try {
        const NJson::TJsonValue json = JsonFromString(responseProto.GetContent());
        if (!json.IsMap()) {
            return TError{TError::EType::Critical} << "Translate API returned broken json value";
        }
        if (json.Has("error")) {
            return TError{TError::EType::Critical} << "Translate API returned error: "
                                                   << json["error"].GetStringSafe();
        }
        const NJson::TJsonValue::TArray texts = json["text"].GetArraySafe();
        if (texts.empty()) {
            return TError{TError::EType::Critical} << "Translated utterance not found in the response body";
        }
        return texts.front().GetStringSafe();
    } catch (...) { // Any parsing error.
        return TError{TError::EType::Parse} << "Misspell response parsing error: " << CurrentExceptionMessage();
    }
}

} // namespace

TSourcePrepareStatus GetUtteranceFromEvent(TRequestComponentsView<TEventComponent> view, TString& utterance) {
    const auto event = view.EventWrapper();
    if (!event) {
        return TError{TError::EType::DataError} << "No event found";
    }

    if (!event->IsTextInput() && !event->IsVoiceInput()) {
        return ESourcePrepareType::NotNeeded;
    }

    if (event->GetUtterance().Empty()) {
        return TError{TError::EType::Empty} << "Couldn't fetch utterance for translation";
    }

    utterance = event->GetUtterance();
    return ESourcePrepareType::Succeeded;
}

// TODO: Translate all ASR hypotheses, not only winning
// Details: https://st.yandex-team.ru/MEGAMIND-2848#612e046155b6e643b19a2fb8
TStatus AppHostPolyglotSetup(IAppHostCtx& ahCtx, TRequestComponentsView<TClientComponent, TEventComponent> request,
                             TMaybe<TString>& utterance, TStringBuf languagePair) {
    TString originalUtterance;
    const auto result = GetUtteranceFromEvent(request, originalUtterance);
    if (!result.IsSuccess()) {
        return result.Status();
    }
    if (result.Value() == ESourcePrepareType::NotNeeded) {
        return Success();
    }

    TString userGender;
    const NScenarios::TUserClassification userClassification =
        ParseUserClassification(request.Event().GetBiometryClassification());
    if (userClassification.GetGender() == NScenarios::TUserClassification::Female) {
        userGender = "feminine";
    } else {
        userGender = "masculine";
    }
    TString contextPrompts = TString::Join("first_person:", userGender, ",second_person:feminine");

    TAppHostHttpProxyMegamindRequestBuilder requestBuilder;
    requestBuilder.AddCgiParam("text", originalUtterance);
    requestBuilder.AddCgiParam("lang", languagePair);
    requestBuilder.AddCgiParam("srv", "alice");
    requestBuilder.AddCgiParam("context_prompts", contextPrompts);
    requestBuilder.CreateAndPushRequest(ahCtx, AH_ITEM_POLYGLOT_HTTP_REQUEST_NAME);

    // We sent a request to the polyglot, and want to prevent fall through UtterancePostSetup
    utterance.Clear();

    return Success();
}

TStatus AppHostPolyglotPostSetup(IAppHostCtx& ahCtx, TPolyglotTranslateUtteranceResponse& polyglotTranslateUtteranceResponse) {
    TString utterance;
    if (auto err = GetTranslationFromContext(ahCtx).MoveTo(utterance)) {
        LOG_WARN(ahCtx.Log()) << "Failed to retrieve polyglot translated utterance: " << *err;
        return std::move(*err);
    }

    LOG_DEBUG(ahCtx.Log()) << "Utterance translated successfully: " << utterance;
    polyglotTranslateUtteranceResponse = TPolyglotTranslateUtteranceResponse(utterance);

    return Success();
}

} // namespace NAlice::NMegamind
