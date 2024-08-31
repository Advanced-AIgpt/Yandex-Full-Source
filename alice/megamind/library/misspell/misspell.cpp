#include "misspell.h"
#include "utils.h"

#include <alice/library/json/json.h>

namespace NAlice::NMegamind {
namespace {

i64 ParseConfidence(const NJson::TJsonValue& json) {
    i64 confidence = 0;
    if (json.IsMap() && json.Has("r") && json["r"].IsInteger()) {
        confidence = json["r"].GetInteger();
    }
    return confidence;
}

TMaybe<TString> ParseFixedText(const NJson::TJsonValue& json, i64 confidence) {
    constexpr i64 ConfidentMisspellCode = 10000;

    if (confidence != ConfidentMisspellCode || !json.IsMap()) {
        return Nothing();
    }

    const auto& text = json["text"];
    if (!text.IsString()) {
        return Nothing();
    }

    return NMisspell::CleanMisspellMarkup(text.GetString());
}

} // namespace

TSourcePrepareStatus CreateMisspellRequest(const TString& utterance, bool processMisspell, NNetwork::IRequestBuilder& request) {
    if (!processMisspell) {
        return ESourcePrepareType::NotNeeded;
    }

    if (utterance.Empty()) {
        return TError() << "Could not fetch Misspell source: utterance is empty";
    }

    request.AddCgiParam(TStringBuf("text"), utterance);
    request.AddCgiParam(TStringBuf("srv"), TStringBuf("vins"));

    return ESourcePrepareType::Succeeded;
}

TErrorOr<TMisspellProto> ParseMisspellResponse(TStringBuf content) {
    try {
        TMisspellProto misspellResponse;
        const NJson::TJsonValue json = JsonFromString(content);
        misspellResponse.SetConfidence(ParseConfidence(json));
        if (auto text = ParseFixedText(json, misspellResponse.GetConfidence())) {
            misspellResponse.SetFixedText(*text);
        }
        return misspellResponse;
    } catch (...) { // Any parsing error.
        return TError() << "Misspell response parsing error: " << CurrentExceptionMessage();
    }
}

} // namespace NAlice::NMegamind
