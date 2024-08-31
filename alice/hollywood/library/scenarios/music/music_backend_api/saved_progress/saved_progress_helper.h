#pragma once

#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>

#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <library/cpp/json/writer/json_value.h>

namespace NAlice::NHollywood::NMusic {

void TryAddCanStartFromTheBeginningAttention(
    TRTLogger& logger,
    const TScenarioApplyRequestWrapper& applyRequest,
    const TMusicArguments& applyArgs,
    TResponseBodyBuilder& bodyBuilder,
    NJson::TJsonValue& stateJson);

} // namespace NAlice::NHollywood::NMusic
