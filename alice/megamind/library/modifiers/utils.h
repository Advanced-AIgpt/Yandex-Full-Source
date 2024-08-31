#pragma once

#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/models/directives/memento_change_user_objects_directive_model.h>
#include <alice/megamind/library/models/directives/update_datasync_directive_model.h>

#include <util/generic/strbuf.h>

namespace NAlice::NMegamind {

void AddTextToResponse(TScenarioResponse& response, const TString& text,
                       const TString& tts, bool appendTts = false);
void AddTextToResponse(TScenarioResponse& response, const TString& text, bool appendTts = false);

TStringBuf GetTextFromResponse(const TScenarioResponse& response);

TStringBuf GetSpeechFromResponse(const TScenarioResponse& response);

bool GetShouldListenFromResponse(const TScenarioResponse& response);

void SetShouldListenToResponse(TScenarioResponse& response, bool shouldListen);

void AddDirectiveToResponse(TScenarioResponse& response,
                            TIntrusivePtr<TUniproxyDirectiveModel>&& directive);

void AddMementoDirectiveToResponse(TScenarioResponse& response,
                                   ru::yandex::alice::memento::proto::EConfigKey userConfigKey,
                                   const google::protobuf::Message& data);

void AddDatasyncDirectiveToResponse(TScenarioResponse& response,
                                    TStringBuf key, TStringBuf value,
                                    EUpdateDatasyncMethod method = EUpdateDatasyncMethod::Put);

template <typename T>
void AddActionToResponse(TScenarioResponse& response, TStringBuf name, T&& action) {
    if (auto* builder = response.BuilderIfExists()) {
        builder->PutAction(TString{name}, std::forward<T>(action));
    }
}

} // NAlice::NMegamind
