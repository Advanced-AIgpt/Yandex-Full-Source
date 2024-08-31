#pragma once

#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

#include <alice/library/logger/logger.h>

#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice::NHollywood::NMusic {

inline constexpr TStringBuf MUSIC_ONBOARDING_FRAME = "alice.music_onboarding";
inline constexpr TStringBuf MUSIC_ONBOARDING_ARTISTS_FRAME = "alice.music_onboarding.artists";
inline constexpr TStringBuf MUSIC_ONBOARDING_GENRES_FRAME = "alice.music_onboarding.genres";
inline constexpr TStringBuf MUSIC_ONBOARDING_TRACKS_FRAME = "alice.music_onboarding.tracks";
inline constexpr TStringBuf MUSIC_ONBOARDING_TRACKS_REASK_FRAME = "alice.music_onboarding.tracks_reask";
inline constexpr TStringBuf MUSIC_ONBOARDING_DONT_KNOW_FRAME = "alice.music_onboarding.dont_know";

inline constexpr TStringBuf MUSIC_ONBOARDING_SILENCE_INTENT = "alice.music_onboarding.silence";

inline constexpr TStringBuf MUSIC_COMPLEX_LIKE_DISLIKE_INTENT = "alice.music.complex_like_dislike";

using TAskingFavorite = TOnboardingState::TAskingFavorite;

struct TOnboardingResponseParams {
    TOnboardingResponseParams()
        : Intent{TString{MUSIC_COMPLEX_LIKE_DISLIKE_INTENT}}
        , StopOnNoReask{true}
        , TryReask{false}
        , TryReaskTrack{false}
        , FillPlayerFeatures{true}
        , TryNextMasterOnboardingStage{false}
        , AddMasterPrephrase{false}
    {}

    TString Intent;
    bool StopOnNoReask;
    bool TryReask;
    bool TryReaskTrack;
    bool FillPlayerFeatures;
    bool TryNextMasterOnboardingStage;
    bool AddMasterPrephrase;
    TVector<NScenarios::TAnalyticsInfo::TAction> AnalyticsActions;
};

class TOnboardingResponseParamsBuilder {
public:
    TOnboardingResponseParamsBuilder() = default;

    TOnboardingResponseParamsBuilder& SetIntent(const TString& intent);
    TOnboardingResponseParamsBuilder& SetStopOnNoReask(bool stop);
    TOnboardingResponseParamsBuilder& SetTryReask(bool reask);
    TOnboardingResponseParamsBuilder& SetTryReaskTrack(bool reaskTrack);
    TOnboardingResponseParamsBuilder& SetFillPlayerFeatures(bool fillPlayerFeatures);
    TOnboardingResponseParamsBuilder& SetTryNextMasterOnboardingStage(bool tryNextMasterOnboardingStage);
    TOnboardingResponseParamsBuilder& SetAddMasterPrephrase(bool addPrephrase);
    TOnboardingResponseParamsBuilder& AddAction(const TString& id, const TString& name, const TString& description);

    TOnboardingResponseParams Build();

private:
    TOnboardingResponseParams Params;
};

void FillLikeDislikeResponse(THwFrameworkRunResponseBuilder& response, TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                             TScenarioState& scState, TMusicArguments& applyArgs);

void FillOnboardingResponse(THwFrameworkRunResponseBuilder& response, TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                            TScenarioState& scState, const TString& phrase, const TOnboardingResponseParams& params = {});

NScenarios::TScenarioRunResponse CreateOnboardingResponse(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                          TNlgWrapper& nlg, TScenarioState& scState, const TString& phrase,
                                                          const TOnboardingResponseParams& params = {});

NScenarios::TScenarioRunResponse HandleMusicOnboardingArtists(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                              TNlgWrapper& nlg, TScenarioState& scState);

NScenarios::TScenarioRunResponse HandleMusicOnboardingGenres(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                             TNlgWrapper& nlg, TScenarioState& scState);

NScenarios::TScenarioRunResponse HandleMusicOnboardingTracks(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                             TNlgWrapper& nlg, TScenarioState& scState);

NScenarios::TScenarioRunResponse HandleMusicOnboardingTracksReask(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                                  TNlgWrapper& nlg, TScenarioState& scState);

NScenarios::TScenarioRunResponse HandleMusicMasterOnboarding(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                             TNlgWrapper& nlg, TScenarioState& scState);

NScenarios::TScenarioRunResponse HandleMusicOnboardingRepeatedSkipDecline(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                                          TNlgWrapper& nlg, TScenarioState& scState);

bool ShouldFillDefaultOnboardingAction(const TScenarioRunRequestWrapper& request, const TFrame& frame,
                                       const TOnboardingState& onboardingState);

bool InTracksGame(TRTLogger& logger, const TDeviceState& deviceState, TScenarioState& scState);

bool IsMusicOnboardingOnPlayDeclineCallback(TStringBuf name);

bool IsMusicOnboardingOnTracksGameDeclineCallback(TStringBuf name);

bool IsMusicOnboardingOnDontKnowCallback(const TStringBuf name);

bool IsMusicOnboardingOnSilenceCallback(const TStringBuf name);

bool IsMusicOnboardingOnRepeatedSkipDeclineCallback(TStringBuf name);

} // namespace NAlice::NHollywood::NMusic
