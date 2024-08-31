#pragma once

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood::NMusic {

constexpr TStringBuf PLAYER_NEXT_TRACK_INTENT = "personal_assistant.scenarios.player_next_track";
constexpr TStringBuf PLAYER_PREV_TRACK_INTENT = "personal_assistant.scenarios.player_previous_track";
constexpr TStringBuf MUSIC_PLAYER_CHANGE_TRACK_NUMBER_INTENT = "alice.music.change_track_number";
constexpr TStringBuf MUSIC_PLAYER_CHANGE_TRACK_VERSION_INTENT = "alice.music.change_track_version";
constexpr TStringBuf PLAYER_CONTINUE_INTENT = "personal_assistant.scenarios.player_continue";
constexpr TStringBuf PLAYER_WHAT_IS_PLAYING_INTENT = "personal_assistant.scenarios.music_what_is_playing";
constexpr TStringBuf PLAYER_LIKE_INTENT = "personal_assistant.scenarios.player_like";
constexpr TStringBuf PLAYER_DISLIKE_INTENT = "personal_assistant.scenarios.player_dislike";
constexpr TStringBuf PLAYER_SHUFFLE_INTENT = "personal_assistant.scenarios.player_shuffle";
constexpr TStringBuf PLAYER_REPLAY_INTENT = "personal_assistant.scenarios.player_replay";
constexpr TStringBuf PLAYER_REWIND_INTENT = "personal_assistant.scenarios.player_rewind";
constexpr TStringBuf PLAYER_REPEAT_INTENT = "personal_assistant.scenarios.player_repeat";

TInstant AnalyticsInfoInstant(const TScenarioBaseRequestWrapper& request);

void FillAnalyticsInfoForMusicPlaySimple(NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder);

void FillAnalyticsInfoSelectedSourceEvent(NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder,
    const TScenarioBaseRequestWrapper& request);

void FillAnalyticsInfoVinsErrorMeta(NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder,
    const TScenarioBaseRequestWrapper& request,
    const TString& errorType);

void FillAnalyticsInfoRadio(NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder,
    const TScenarioBaseRequestWrapper& request);

void FillAnalyticsInfoFromContinueArgs(NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder,
    const TMusicArguments& musicArgs);

void FillAnalyticsInfoFromWebAnswer(NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder,
    const TScenarioBaseRequestWrapper& request,
    const NJson::TJsonValue& webAnswer);

void FillAnalyticsInfoMusicEvent(TRTLogger& logger,
    const NJson::TJsonValue& bassResponse,
    TResponseBodyBuilder* responseBodyBuilder,
    const TScenarioBaseRequestWrapper& baseRequest);

void FillAnalyticsInfoForMusicPlay(const TContentId& contentId,
    const TQueueItem& curItem,
    ui64 serverTimeMs,
    NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder,
    bool printAllArtistsInHumanReadable,
    bool onboardingTracksGame = false,
    bool isFairyTalesFrame = false);

void CreateAndFillAnalyticsInfoForMusicPlay(TRTLogger& logger,
                                            const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                            const TMusicQueueWrapper& mq, const TMusicArguments& applyArgs,
                                            TResponseBodyBuilder& bodyBuilder,
                                            const TString& parentProductScenarioName,
                                            bool onboardingTracksGame = false, bool batchOfTracksRequested = false,
                                            bool cacheHit = false);

void CreateAndFillAnalyticsInfoForNextPlayCallback(TRTLogger& logger,
                                                   const TMusicQueueWrapper& mq,
                                                   TResponseBodyBuilder& bodyBuilder);

void CreateAndFillAnalyticsInfoForPlayerCommand(TMusicArguments::EPlayerCommand playerCommand,
                                                TResponseBodyBuilder& bodyBuilder,
                                                ui64 serverTimeMs,
                                                const TString& parentProductScenarioName = Default<TString>(),
                                                bool isMusicPlaying = true,
                                                bool batchOfTracksRequested = false,
                                                bool cacheHit = false);

void CreateAndFillAnalyticsInfoForPlayerCommandWithFirstTrackObject(TMusicArguments::EPlayerCommand playerCommand,
    const TQueueItem& curItem,
    TResponseBodyBuilder& bodyBuilder,
    ui64 serverTimeMs,
    const TString& parentProductScenarioName = Default<TString>(),
    bool batchOfTracksRequested = false,
    bool cacheHit = false);

void CreateAndFillAnalyticsInfoForPlayerCommandRewind(TResponseBodyBuilder& bodyBuilder,
                                                      const NScenarios::TAudioRewindDirective::EType rewindType,
                                                      ui32 rewindMs,
                                                      ui64 serverTimeMs,
                                                      const TString& parentProductScenarioName = Default<TString>(),
                                                      bool batchOfTracksRequested = false,
                                                      bool cacheHit = false);

void CreateAndFillAnalyticsInfoForPlayerCommandBeforeAutoflow(
    TMusicArguments::EPlayerCommand playerCommand,
    const TQueueItem& curItem,
    TResponseBodyBuilder& bodyBuilder,
    ui64 serverTimeMs,
    const TString& parentProductScenarioName = Default<TString>(),
    bool batchOfTracksRequested = false,
    bool cacheHit = false);
} // namespace NAlice::NHollywood::NMusic
