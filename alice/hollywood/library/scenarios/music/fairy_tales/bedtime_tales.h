#pragma once

#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

#include <alice/library/logger/logger.h>

#include <library/cpp/json/writer/json_value.h>

#include <util/generic/string.h>

namespace NAlice::NHollywood::NMusic {

const ui64 BEDTIME_TALES_PLAY_DURATION_SEC = 900;

bool IsBedtimeTales(
    const TScenarioInputWrapper& requestInput,
    bool bedtimeTalesExp);

bool CheckFairyTalesStopTimer(
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
    TScenarioState& scState);

void SetBedtimeTalesState(
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
    TScenarioState& scState,
    bool isBedtimeTales);

bool ShouldAddBedtimeTalesOnboarding(
    TRTLogger& logger,
    const TScenarioApplyRequestWrapper& applyRequest);

void AddBedtimeTalesOnboardingAttention(
    NJson::TJsonValue& stateJson,
    bool isThinPlayer);

void AddBedtimeTalesOnboardingDirective(
    TRTLogger& logger,
    const TScenarioApplyRequestWrapper& applyRequest,
    TResponseBodyBuilder& bodyBuilder);

} // namespace NAlice::NHollywood::NMusic
