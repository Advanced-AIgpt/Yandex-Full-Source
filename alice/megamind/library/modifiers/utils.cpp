#include "utils.h"

namespace NAlice::NMegamind {

void AddTextToResponse(TScenarioResponse& response, const TString& text,
                       const TString& tts, bool appendTts) {
    if (auto* builder = response.BuilderIfExists()) {
        builder->AddSimpleText(text, tts, appendTts);
    }
}
void AddTextToResponse(TScenarioResponse& response, const TString& text, bool appendTts) {
    AddTextToResponse(response, text, text, appendTts);
}

TStringBuf GetTextFromResponse(const TScenarioResponse& response) {
    if (auto* builder = response.BuilderIfExists()) {
        return builder->GetRenderedText();
    }
    Y_UNREACHABLE();
}

TStringBuf GetSpeechFromResponse(const TScenarioResponse& response) {
    if (auto* builder = response.BuilderIfExists()) {
        return builder->GetRenderedSpeech();
    }
    Y_UNREACHABLE();
}

bool GetShouldListenFromResponse(const TScenarioResponse& response) {
    if (auto* builder = response.BuilderIfExists()) {
        return builder->GetShouldListen(/* default= */ true);
    }
    Y_UNREACHABLE();
}

void SetShouldListenToResponse(TScenarioResponse& response, bool shouldListen) {
    if (auto* builder = response.BuilderIfExists()) {
        builder->ShouldListen(shouldListen);
    }
}

void AddDirectiveToResponse(TScenarioResponse& response,
                            TIntrusivePtr<TUniproxyDirectiveModel>&& directive) {
    Y_ASSERT(directive);
    if (auto* builder = response.BuilderIfExists()) {
        builder->AddDirectiveToVoiceResponse(*directive);
    }
}

void AddMementoDirectiveToResponse(TScenarioResponse& response,
                                   ru::yandex::alice::memento::proto::EConfigKey userConfigKey,
                                   const google::protobuf::Message& data) {
    AddDirectiveToResponse(response,
                           MakeIntrusive<TMementoChangeUserObjectsDirectiveModel>(userConfigKey, data));
}

void AddDatasyncDirectiveToResponse(TScenarioResponse& response,
                                    TStringBuf key, TStringBuf value,
                                    EUpdateDatasyncMethod method) {
    AddDirectiveToResponse(response,
                           MakeIntrusive<TUpdateDatasyncDirectiveModel>(TString{key}, TString{value}, method));
}

} // NAlice::NMegamind
