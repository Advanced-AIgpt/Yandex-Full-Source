#pragma once

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

namespace NAlice::NHollywood::NMusic {

TMaybe<TStringBuf> GetMusicPlayerFrame(const TScenarioRunRequestWrapper& request);

std::variant<THttpProxyRequest, NScenarios::TScenarioRunResponse, TString>
HandleMusicPlayerCommand(NAlice::TRTLogger& logger,
                         const TScenarioRunRequestWrapper& request,
                         const NScenarios::TRequestMeta& meta,
                         TNlgWrapper& nlg,
                         const TStringBuf playerFrame,
                         const NJson::TJsonValue& appHostParams);

} // NAlice::NHollywood
