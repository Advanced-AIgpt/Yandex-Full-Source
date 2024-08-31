#include "onboarding.h"

#include "common.h"

#include <alice/hollywood/library/nlg/nlg_data.h>
#include <alice/hollywood/library/scenarios/music/util/onboarding.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/consts.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>

#include <alice/megamind/protos/common/atm.pb.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/music/defs.h>

namespace NAlice::NHollywood::NMusic {

namespace {

const THashMap<TAskingFavorite::EContentType, TStringBuf> MASTER_ONBOARDING_SUBPHRASES({
    { TAskingFavorite::Artist, "artist__ask" },
    { TAskingFavorite::Genre, "genre__ask" },
});

bool HasNextOnboardingStage(const TScenarioState& scState) {
    return scState.GetOnboardingState().GetInMasterOnboarding() && scState.GetOnboardingState().OnboardingSequenceSize() > 1;
}

void FillOnboardingResponse(THwFrameworkRunResponseBuilder& response, TScenarioHandleContext& ctx, TResponseBodyBuilder& bodyBuilder, TNlgData& nlgData,
                            TScenarioState& scState, const TString& phrase, const TScenarioRunRequestWrapper& request,
                            const TOnboardingResponseParams& params)
{
    auto& logger = ctx.Ctx.Logger();

    if (params.FillPlayerFeatures) {
        // A hack cause we do not care which player was last used
        // TODO(jan-fazli): Remove this hack when player likes/dislikes start using grammars
        response.FillPlayerFeatures(/* restorePlayer= */ true, /* secondsSincePause= */ 0);
        LOG_INFO(logger) << "Filled music onboarding player features";
    }

    auto& analyticsInfoBuilder = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetIntentName(params.Intent);
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::MUSIC);
    analyticsInfoBuilder.AddActions(params.AnalyticsActions);
    LOG_INFO(logger) << "Filled music onboarding analytics";
    response.SetFeaturesIntent(params.Intent);

    if (params.AddMasterPrephrase) {
        nlgData.Context["prephrase"] = "master__start";
        LOG_INFO(logger) << "Starting master music onboarding";
    }

    const bool isAskingFavorite = scState.HasOnboardingState() && IsAskingFavorite(scState.GetOnboardingState());

    bool reask = false;
    if (isAskingFavorite) {
        auto& askingFavorite = *scState.MutableOnboardingState()->MutableOnboardingSequence(0)->MutableAskingFavorite();
        reask = params.TryReask && !askingFavorite.GetReasking(); // Do not reask twice
        askingFavorite.SetReasking(reask);

        nlgData.Context["subphrase_repeat"] = MASTER_ONBOARDING_SUBPHRASES.at(askingFavorite.GetType());

        {
            NScenarios::TFrameAction dontKnowAction;
            dontKnowAction.MutableNluHint()->SetFrameName(TString{MUSIC_ONBOARDING_DONT_KNOW_FRAME});
            dontKnowAction.MutableCallback()->SetName(TString{MUSIC_ONBOARDING_ON_DONT_KNOW_CALLBACK});
            bodyBuilder.AddAction("music_onboarding_dont_know", std::move(dontKnowAction));
        }
    }

    bool stop = false;
    if (reask) {
        nlgData.Context["reask"] = true;
        LOG_INFO(logger) << "Reasking in music onboarding";
    } else if (params.TryNextMasterOnboardingStage && HasNextOnboardingStage(scState)) {
        const auto& nextOnboardingStage = scState.GetOnboardingState().GetOnboardingSequence(1);
        if (nextOnboardingStage.HasAskingFavorite()) {
            const auto& nextAskingFavoriteType = nextOnboardingStage.GetAskingFavorite().GetType();
            Y_ENSURE(MASTER_ONBOARDING_SUBPHRASES.contains(nextAskingFavoriteType));
            nlgData.Context["subphrase"] = MASTER_ONBOARDING_SUBPHRASES.at(nextAskingFavoriteType);
            LOG_INFO(logger) << "Advancing master onboarding to asking favorite " << TAskingFavorite::EContentType_Name(nextAskingFavoriteType);
        } else {
            Y_ENSURE(nextOnboardingStage.HasTracksGame());
            // Tracks game should always be last for now, cause we can not move to a different stage after it
            Y_ENSURE(scState.GetOnboardingState().OnboardingSequenceSize() == 2);

            {
                NScenarios::TFrameAction confirmAction;
                confirmAction.MutableNluHint()->SetFrameName(NMusic::HINT_FRAME_CONFIRM);
                auto& parsed = *confirmAction.MutableParsedUtterance();
                parsed.MutableAnalytics()->SetPurpose("Start onboarding tracks game");
                parsed.MutableTypedSemanticFrame()->MutableMusicOnboardingTracksSemanticFrame();
                bodyBuilder.AddAction("music_onboarding_tracks_game_confirm", std::move(confirmAction));
            }
            {
                NScenarios::TFrameAction declineAction;
                declineAction.MutableNluHint()->SetFrameName(NMusic::HINT_FRAME_DECLINE);
                declineAction.MutableCallback()->SetName(TString{NMusic::MUSIC_ONBOARDING_ON_TRACKS_GAME_DECLINE_CALLBACK});
                bodyBuilder.AddAction("music_onboarding_tracks_game_decline", std::move(declineAction));
            }

            nlgData.Context["subphrase"] = "track__game_ask";
            LOG_INFO(logger) << "Advancing master onboarding to tracks game";
        }
        // Advance the onboarding sequence
        scState.MutableOnboardingState()->MutableOnboardingSequence()->DeleteSubrange(0, 1);
    } else if (params.TryReaskTrack) {
        auto& tracksGame = *scState
            .MutableOnboardingState()
            ->MutableOnboardingSequence(0)
            ->MutableTracksGame();
        const auto reaskCount = tracksGame.GetReaskCount();
        if (reaskCount > 0) {
            tracksGame.SetReaskCount(reaskCount - 1);
        }
        if (reaskCount > 1) {
            bodyBuilder.AddServerDirective(
                MakeTracksMidrollDirective(request, GetUid(request), tracksGame.GetTrackIndex())
            );
        }
        LOG_INFO(logger) << "Reasking in tracks game";
    } else if (params.StopOnNoReask) {
        stop = true;
        scState.ClearOnboardingState();
        LOG_INFO(logger) << "Exiting music onboarding";
    } else {
        LOG_INFO(logger) << "Default music onboarding flow (probably asking something for the first time)";
    }

    if (!stop) {
        bodyBuilder.SetExpectsRequest(true);

        if (request.Interfaces().GetHasDirectiveSequencer()) {
            bodyBuilder.AddListenDirective();

            if (isAskingFavorite) {
                bodyBuilder.ResetAddBuilder()
                    .AddCallback(TString{MUSIC_ONBOARDING_ON_SILENCE_CALLBACK}, TDirectiveChannel::Content, {})
                    .SetIsLedSilent(true);
            }
        } else {
            bodyBuilder.SetShouldListen(true);
        }
    }

    bodyBuilder.SetState(scState);
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_ONBOARDING, phrase, /* buttons = */ {}, nlgData);
}

class TOnboardingSequenceBuilder {
public:
    explicit TOnboardingSequenceBuilder(NMusic::TOnboardingState& onboardingState)
        : State(onboardingState)
    {
        State.ClearOnboardingSequence();
    }

    TOnboardingSequenceBuilder& AddAskFavorite(const TAskingFavorite::EContentType type) {
        State.AddOnboardingSequence()->MutableAskingFavorite()->SetType(type);
        return *this;
    }

    TOnboardingSequenceBuilder& AddTracksGame(const ui32 tracksCount) {
        State.AddOnboardingSequence()->MutableTracksGame()->SetTracksCount(tracksCount);
        return *this;
    }

private:
    TOnboardingState& State;
};

NScenarios::TScenarioRunResponse CreateOnboardingContinueWithRadioResponse(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                                           TNlgWrapper& nlg, const TOnboardingState& onboardingState)
{
    auto& logger = ctx.Ctx.Logger();
    THwFrameworkRunResponseBuilder response{ctx, &nlg};

    auto args = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         /* isNewContentRequested= */ true);
    auto& radioRequest = *args.MutableRadioRequest();
    if (request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ONBOARDING_GENRE_RADIO) && onboardingState.GetInMasterOnboarding() && onboardingState.GetLikedGenre()) {
        radioRequest.AddStationIds(TString::Join(NAlice::NMusic::SLOT_GENRE, ":", onboardingState.GetLikedGenre()));
    } else if (request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ONBOARDING_DISCOVERY_RADIO)) {
        radioRequest.AddStationIds(NMusic::DISCOVERY_RADIO_STATION_ID);
    } else {
        radioRequest.AddStationIds(NMusic::USER_RADIO_STATION_ID);
    }
    args.SetOnboardingTracksGame(true);

    const TStringBuf uid = GetUid(request);
    args.MutableAccountStatus()->SetUid(uid.data(), uid.size());
    response.SetContinueArguments(args);

    return *std::move(response).BuildResponse();
}

NScenarios::TScenarioRunResponse HandlingMusicOnboarding(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg,
                                                         TScenarioState& scState, const TString& intent, const TString& phrase = "",
                                                         TOnboardingResponseParamsBuilder&& responseParamsBuilder = TOnboardingResponseParamsBuilder{})
{
    auto& logger = ctx.Ctx.Logger();

    responseParamsBuilder.SetIntent(intent).SetFillPlayerFeatures(false);

    if (!HasMusicSubscription(request)) {
        LOG_WARNING(logger) << "Can not set music recommendations without MusicSubscription";
        return CreateOnboardingResponse(
            ctx, request, nlg, scState, "no_subscription__onboarding",
            responseParamsBuilder.Build());
    }

    auto& onboardingState = *scState.MutableOnboardingState();

    // No phrase indicated that we need to start the tracks game
    if (!phrase) {
        // We do not change state for going in continue, cause it will not be saved anyway
        return CreateOnboardingContinueWithRadioResponse(ctx, request, nlg, onboardingState);
    }

    onboardingState.SetInOnboarding(true);
    // Should be added before calling this function with a phrase
    Y_ENSURE(onboardingState.OnboardingSequenceSize());
    responseParamsBuilder.SetStopOnNoReask(false);
    return CreateOnboardingResponse(ctx, request, nlg, scState, phrase, responseParamsBuilder.Build());
}

} // namespace

TOnboardingResponseParamsBuilder& TOnboardingResponseParamsBuilder::SetIntent(const TString& intent) {
    Params.Intent = intent;
    return *this;
}

TOnboardingResponseParamsBuilder& TOnboardingResponseParamsBuilder::SetStopOnNoReask(const bool stop) {
    Params.StopOnNoReask = stop;
    return *this;
}

TOnboardingResponseParamsBuilder& TOnboardingResponseParamsBuilder::SetTryReask(const bool reask) {
    Params.TryReask = reask;
    return *this;
}

TOnboardingResponseParamsBuilder& TOnboardingResponseParamsBuilder::SetTryReaskTrack(const bool reaskTrack) {
    Params.TryReaskTrack = reaskTrack;
    return *this;
}

TOnboardingResponseParamsBuilder& TOnboardingResponseParamsBuilder::SetFillPlayerFeatures(const bool fillPlayerFeatures) {
    Params.FillPlayerFeatures = fillPlayerFeatures;
    return *this;
}

TOnboardingResponseParamsBuilder& TOnboardingResponseParamsBuilder::SetTryNextMasterOnboardingStage(
    const bool tryNextMasterOnboardingStage
) {
    Params.TryNextMasterOnboardingStage = tryNextMasterOnboardingStage;
    return *this;
}

TOnboardingResponseParamsBuilder& TOnboardingResponseParamsBuilder::SetAddMasterPrephrase(const bool addPrephrase) {
    Params.AddMasterPrephrase = addPrephrase;
    return *this;
}

TOnboardingResponseParamsBuilder& TOnboardingResponseParamsBuilder::AddAction(const TString& id, const TString& name, const TString& description) {
    NScenarios::TAnalyticsInfo::TAction action;
    action.SetId(id);
    action.SetName(name);
    action.SetHumanReadable(description);
    Params.AnalyticsActions.emplace_back(std::move(action));
    return *this;
}

TOnboardingResponseParams TOnboardingResponseParamsBuilder::Build() {
    return std::move(Params);
}

void FillLikeDislikeResponse(THwFrameworkRunResponseBuilder& response, TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                             TScenarioState& scState, TMusicArguments& applyArgs)
{
    auto& logger = ctx.Ctx.Logger();

    Y_ENSURE(applyArgs.HasComplexLikeDislikeRequest());
    const auto& likeDislike = applyArgs.GetComplexLikeDislikeRequest();
    const bool isLike = likeDislike.GetIsLike();
    const auto likeDislikeStr = isLike ? "like" : "dislike";

    TString contentType;
    auto contentAnalytics = TStringBuilder{} << "Ставится " << (isLike ? "лайк" : "дизлайк") << " ";
    TNlgData nlgData{logger, request};
    if (likeDislike.HasArtistTarget()) {
        contentType = "artist";
        contentAnalytics << "исполнителю " << likeDislike.GetArtistTarget().GetName();
        // For artist likes we say the artist's name
        if (isLike && likeDislike.GetArtistTarget().GetName()) {
            nlgData.Context["artist_name"] = likeDislike.GetArtistTarget().GetName();
        }
    } else if (likeDislike.HasTrackTarget()) {
        contentType = "track";
        contentAnalytics << "треку " << likeDislike.GetTrackTarget().GetTitle();
    } else if (likeDislike.HasAlbumTarget()) {
        Y_ENSURE(isLike); // Album dislikes are unsupported
        contentType = "album";
        contentAnalytics << "альбому " << likeDislike.GetAlbumTarget().GetTitle();
    } else {
        Y_ENSURE(likeDislike.HasGenreTarget());
        Y_ENSURE(isLike); // Genre dislikes are unsupported
        contentType = "genre";
        // TODO(jan-fazli): Maybe someday get russian genre name from somewhere
        contentAnalytics << "жанру " << likeDislike.GetGenreTarget().GetId();
        // For genre we say its title
        nlgData.Context["genre_id"] = likeDislike.GetGenreTarget().GetId();
        // This may be used for tracks game to play genre radio
        scState.MutableOnboardingState()->SetLikedGenre(likeDislike.GetGenreTarget().GetId());
    }

    auto& bodyBuilder = response.CreateCommitCandidate(std::move(applyArgs));
    auto paramsBuilder = TOnboardingResponseParamsBuilder{}
        .SetTryReask(!isLike)
        .SetTryNextMasterOnboardingStage(true)
        .AddAction(
            TString::Join("player_", likeDislikeStr, "_", contentType),
            TString::Join("player ", likeDislikeStr, " ", contentType),
            contentAnalytics);

    FillOnboardingResponse(
        response, ctx, bodyBuilder, nlgData, scState,
        TString::Join(contentType, "__", likeDislikeStr),
        request, paramsBuilder.Build());

    LOG_INFO(logger) << "Rendered complex like/dislike, proceed to commit";
}

void FillOnboardingResponse(THwFrameworkRunResponseBuilder& response, TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                            TScenarioState& scState, const TString& phrase, const TOnboardingResponseParams& params)
{
    auto& logger = ctx.Ctx.Logger();

    TNlgData nlgData{logger, request};
    auto& bodyBuilder = response.CreateResponseBodyBuilder();
    FillOnboardingResponse(response, ctx, bodyBuilder, nlgData, scState, phrase, request, params);
}

NScenarios::TScenarioRunResponse CreateOnboardingResponse(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                          TNlgWrapper& nlg, TScenarioState& scState, const TString& phrase,
                                                          const TOnboardingResponseParams& params)
{
    THwFrameworkRunResponseBuilder response{ctx, &nlg};
    FillOnboardingResponse(response, ctx, request, scState, phrase, params);
    return *std::move(response).BuildResponse();
}

NScenarios::TScenarioRunResponse HandleMusicOnboardingArtists(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                              TNlgWrapper& nlg, TScenarioState& scState)
{
    auto& logger = ctx.Ctx.Logger();

    LOG_INFO(logger) << "Handling music onboarding artists";
    TOnboardingSequenceBuilder{*scState.MutableOnboardingState()}
        .AddAskFavorite(TAskingFavorite::Artist);
    return HandlingMusicOnboarding(ctx, request, nlg, scState, TString{MUSIC_ONBOARDING_ARTISTS_FRAME}, "artist__ask");
}

NScenarios::TScenarioRunResponse HandleMusicOnboardingGenres(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                              TNlgWrapper& nlg, TScenarioState& scState)
{
    auto& logger = ctx.Ctx.Logger();

    LOG_INFO(logger) << "Handling music onboarding genres";
    TOnboardingSequenceBuilder{*scState.MutableOnboardingState()}
        .AddAskFavorite(TAskingFavorite::Genre);
    return HandlingMusicOnboarding(ctx, request, nlg, scState, TString{MUSIC_ONBOARDING_GENRES_FRAME}, "genre__ask");
}

NScenarios::TScenarioRunResponse HandleMusicOnboardingTracks(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                             TNlgWrapper& nlg, TScenarioState& scState)
{
    auto& logger = ctx.Ctx.Logger();

    LOG_INFO(logger) << "Handling music onboarding tracks";
    Y_ENSURE(IsThinRadioSupported(request));
    // No sequence here cause it will be lost when going to continue stage
    return HandlingMusicOnboarding(ctx, request, nlg, scState, TString{MUSIC_ONBOARDING_TRACKS_FRAME});
}

NScenarios::TScenarioRunResponse HandleMusicOnboardingTracksReask(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                                  TNlgWrapper& nlg, TScenarioState& scState)
{
    auto& logger = ctx.Ctx.Logger();

    LOG_INFO(logger) << "Handling music onboarding tracks reask";
    Y_ENSURE(IsThinRadioSupported(request));

    TOnboardingResponseParamsBuilder responseParamsBuilder;
    responseParamsBuilder.SetTryReaskTrack(true);
    return HandlingMusicOnboarding(ctx, request, nlg, scState, TString{MUSIC_ONBOARDING_TRACKS_REASK_FRAME},
                                   "track__reask", /* responseParamsBuilder= */ std::move(responseParamsBuilder));
}

NScenarios::TScenarioRunResponse HandleMusicMasterOnboarding(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                             TNlgWrapper& nlg, TScenarioState& scState)
{
    auto& logger = ctx.Ctx.Logger();

    LOG_INFO(logger) << "Handling master music onboarding starting with asking favorite " << TAskingFavorite::EContentType_Name(TAskingFavorite::Artist);
    auto& onboardingState = *scState.MutableOnboardingState();
    onboardingState.SetInMasterOnboarding(true);
    TOnboardingSequenceBuilder sequenceBuilder{onboardingState};
    sequenceBuilder.AddAskFavorite(TAskingFavorite::Artist);
    sequenceBuilder.AddAskFavorite(TAskingFavorite::Genre);
    // Music is played with thin client so this part of onboarding is only used when thin client radio is supported
    if (IsThinRadioSupported(request)) {
        sequenceBuilder.AddTracksGame(
            request.LoadValueFromExpPrefix<ui32>(NExperiments::EXP_HW_MUSIC_ONBOARDING_MASTER_TRACKS_COUNT, 3u)
        );
    }
    TOnboardingResponseParamsBuilder responseParamsBuilder;
    responseParamsBuilder.SetAddMasterPrephrase(true);
    return HandlingMusicOnboarding(ctx, request, nlg, scState, TString{MUSIC_ONBOARDING_FRAME}, "artist__ask", /* responseParamsBuilder= */ std::move(responseParamsBuilder));
}

NScenarios::TScenarioRunResponse HandleMusicOnboardingRepeatedSkipDecline(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                                          TNlgWrapper& nlg, TScenarioState& scState)
{
    auto& logger = ctx.Ctx.Logger();

    THwFrameworkRunResponseBuilder response{ctx, &nlg};
    auto& bodyBuilder = response.CreateResponseBodyBuilder();

    const TString intent{MUSIC_ONBOARDING_TRACKS_FRAME};
    auto& analyticsInfoBuilder = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetIntentName(intent);
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::MUSIC);
    response.SetFeaturesIntent(intent);

    auto& utterance = bodyBuilder.ResetAddBuilder().AddUtterance({});
    auto& analytics = *utterance.MutableAnalytics();
    analytics.SetPurpose("Continue onboarding tracks game");
    analytics.SetOrigin(TAnalyticsTrackingModule::Scenario);
    utterance.MutableTypedSemanticFrame()->MutablePlayerNextTrackSemanticFrame();

    TNlgData nlgData{logger, request};
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_ONBOARDING, "track__repeated_skip_decline", /* buttons = */ {}, nlgData);

    bodyBuilder.SetState(scState);
    return *std::move(response).BuildResponse();
}

bool ShouldFillDefaultOnboardingAction(const TScenarioRunRequestWrapper& request, const TFrame& frame,
                                       const TOnboardingState& onboardingState) {
    return request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ONBOARDING)
        && !frame.FindSlot(NAlice::NMusic::SLOT_ACTION_REQUEST)
        && IsAskingFavorite(onboardingState)
        // Even when asking for artist but given genre we still assume it's
        // the like action. Inappropriate slot will be handled later
        && (frame.FindSlot(NAlice::NMusic::SLOT_SEARCH_TEXT) || frame.FindSlot(NAlice::NMusic::SLOT_GENRE));
}

bool InTracksGame(TRTLogger& logger, const TDeviceState& deviceState, TScenarioState& scState) {
    const bool hasAudioPlayer = deviceState.HasAudioPlayer();
    const auto& onboardingState = scState.GetOnboardingState();
    TMusicQueueWrapper mq(logger, *scState.MutableQueue());
    return onboardingState.GetInOnboarding()
        && onboardingState.OnboardingSequenceSize()
        && onboardingState.GetOnboardingSequence(0).HasTracksGame()
        && hasAudioPlayer && deviceState.GetAudioPlayer().GetPlayerState() == TDeviceState_TAudioPlayer_TPlayerState_Playing
        && mq.IsRadio();
}

bool IsMusicOnboardingOnPlayDeclineCallback(const TStringBuf name) {
    return name == MUSIC_ONBOARDING_ON_PLAY_DECLINE_CALLBACK;
}

bool IsMusicOnboardingOnTracksGameDeclineCallback(const TStringBuf name) {
    return name == MUSIC_ONBOARDING_ON_TRACKS_GAME_DECLINE_CALLBACK;
}

bool IsMusicOnboardingOnDontKnowCallback(const TStringBuf name) {
    return name == MUSIC_ONBOARDING_ON_DONT_KNOW_CALLBACK;
}

bool IsMusicOnboardingOnSilenceCallback(const TStringBuf name) {
    return name == MUSIC_ONBOARDING_ON_SILENCE_CALLBACK;
}

bool IsMusicOnboardingOnRepeatedSkipDeclineCallback(const TStringBuf name) {
    return name == MUSIC_ONBOARDING_ON_REPEATED_SKIP_DECLINE_CALLBACK;
}

} // namespace NAlice::NHollywood::NMusic
