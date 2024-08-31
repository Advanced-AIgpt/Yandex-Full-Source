#pragma once

#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>

#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood::NMusic {

std::unique_ptr<NScenarios::TScenarioRunResponse> HandleThinClientPlayerCommand(TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request,
    TMusicArguments::EPlayerCommand playerCommand,
    TNlgWrapper& nlg,
    const TStringBuf playerFrameName,
    TMaybe<TScenarioState>& scenarioState);

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerLikeCommand(
    TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg, bool tracksGame = false);

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerDislikeCommand(
    TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg, bool tracksGame = false);

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerShuffleCommand(
    TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg);

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerUnshuffleCommand(
    TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg);

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerPrevTrackCommand(
    TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg);

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerNextTrackCommand(
    TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg);

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerContinueCommand(
    TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg);

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerReplayCommand(
    TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg);

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerCommands(
    TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg,
    TMusicArguments_EPlayerCommand playerCommand);

} // NAlice::NHollywood::NMusic
