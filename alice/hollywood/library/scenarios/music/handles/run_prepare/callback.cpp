#include "impl.h"

#include <alice/hollywood/library/scenarios/music/generative/generative.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/content_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/onboarding.h>

#include <alice/library/experiments/flags.h>

namespace NAlice::NHollywood::NMusic::NImpl {

namespace {

THwFrameworkRunResponseBuilder HandleTurnOnRadioCallback(TString contentType, TString contentId,
                                                         const TScenarioRunRequestWrapper& request,
                                                         TNlgWrapper& nlg,
                                                         TScenarioHandleContext& ctx)
{
    auto& logger = ctx.Ctx.Logger();
    THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));

    if (contentType == "playlist") {
        auto playlistId = TPlaylistId::FromString(contentId);
        Y_ENSURE(playlistId, "Failed to create playlistId from " << contentId);
        contentId = playlistId->ToStringForRadio();
    }

    const auto radioStationId = TString::Join(contentType, ":", contentId);
    LOG_INFO(logger) << "Turning on ThinClient radio autoflow, stationId=" << radioStationId;

    auto args = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         /* isNewContentRequestedByUser= */ false);

    auto& radioRequest = *args.MutableRadioRequest();
    radioRequest.AddStationIds(radioStationId);

    const TStringBuf uid = GetUid(request);
    args.MutableAccountStatus()->SetUid(uid.data(), uid.size());

    auto& playbackOptions = *args.MutablePlaybackOptions();
    playbackOptions.SetDisableNlg(true);

    response.SetContinueArguments(args);

    return response;
}

bool IsIgnoredShotsFeedback(const google::protobuf::Struct& payload) {
    auto it = payload.fields().find("events");
    if (it == payload.fields().end() || !it->second.has_list_value()) {
        return false;
    }
    const auto& events = it->second.list_value();
    if (events.values_size() == 0 || !events.values(0).has_struct_value()) {
        return false;
    }
    const auto& event = events.values(0).struct_value();

    it = event.fields().find("shotFeedbackEvent");
    if (it == event.fields().end() || !it->second.has_struct_value()) {
        return false;
    }
    const auto& shotFeedbackEvent = it->second.struct_value();
    it = shotFeedbackEvent.fields().find("type");
    // if type is not specified, then there is no need to do something on this callback
    // shots do not have feedbacks for on_started, on_failed and on_stopped
    return it == shotFeedbackEvent.fields().end() || it->second.string_value() == "Unknown";
}

} // namespace

TMaybe<NScenarios::TScenarioRunResponse> TRunPrepareHandleImpl::HandleCallback() {
    if (const auto* callback = Request_.Input().GetCallback()) {
        const auto callbackName = callback->GetName();
        if (IsMusicThinClientNextTrackCallback(callbackName)) {
            return HandleNextTrackCallback();
        } else if (IsMusicTrackPlayLifeCycleCallback(callbackName)) {
            return HandleTrackPlayLifeCycleCallback();
        } else if (IsMusicThinClientRecoveryCallback(callbackName)) {
            return HandleRecoveryCallback();
        } else if (IsMusicOnboardingOnPlayDeclineCallback(callbackName)) {
            return HandleOnboardingOnPlayDeclineCallback();
        } else if (IsMusicOnboardingOnTracksGameDeclineCallback(callbackName)) {
            return HandleOnboardingOnTracksGameDeclineCallback();
        } else if (IsMusicOnboardingOnDontKnowCallback(callbackName)) {
            return HandleOnboardingOnDontKnowCallback();
        } else if (IsMusicOnboardingOnSilenceCallback(callbackName)) {
            return HandleOnboardingOnSilenceCallback();
        } else if (IsMusicOnboardingOnRepeatedSkipDeclineCallback(callbackName)) {
            return HandleOnboardingOnRepeatedSkipDeclineCallback();
        }
    }
    return Nothing();
}

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::HandleNextTrackCallback() {
    LOG_INFO(Logger_) << "Handling NextTrack player callback...";
    Y_ENSURE(ScenarioState_.Defined());

    if (MusicQueue_.IsGenerative()) {
        TString generativeStationId = MusicQueue_.CurrentItem().GetGenerativeInfo().GetGenerativeStationId();
        // NOTE(klim-roma): as it's NextTrack callback no new content is requested by user
        return *HandleThinClientGenerative(Ctx_, Request_, Nlg_, generativeStationId, /* isNewContentRequestedByUser= */ false);
    }

    if (auto shot = MusicQueue_.GetShotBeforeCurrentItem()) {
        return MakeThinClientDefaultResponseWithEmptyContinueArguments();
    }

    Y_ENSURE(GetScenarioStateOrDummy().HasQueue());
    const auto isAnyRepeatOn = MusicQueue_.GetRepeatType() != RepeatNone;
    if (MusicQueue_.IsCurrentTrackLast() && !isAnyRepeatOn) {
        // Got music_thin_client_get_next after the last track of entity
        // so we turn on radio
        Y_ENSURE(!MusicQueue_.IsGenerative() && !MusicQueue_.IsRadio());
        LOG_INFO(Logger_) << "Got NextTrack callback but current track is last and repeat is off";
        const auto& contentIdProto = MusicQueue_.ContentId();
        TString contentType = ContentTypeToText(contentIdProto.GetType());
        TString contentId = contentIdProto.GetId();
        LOG_INFO(Logger_) << "type and id " << contentType << " "
                         << contentId;

        auto response = HandleTurnOnRadioCallback(contentType, contentId, Request_, Nlg_, Ctx_);
        return *std::move(response).BuildResponse();
    }
    Y_ENSURE(!MusicQueue_.IsCurrentTrackLast() || isAnyRepeatOn);
    if (Request_.HasExpFlag(EXP_HW_MUSIC_GET_NEXTS_USE_APPLY)) {
        return MakeThinClientDefaultResponseWithEmptyApplyArguments();
    } else {
        return MakeThinClientDefaultResponseWithEmptyContinueArguments();
    }
}

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::HandleTrackPlayLifeCycleCallback() {
    const auto* callback = Request_.Input().GetCallback();
    const auto callbackName = callback->GetName();

    if (!Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT) && !Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_GENERATIVE)) {
        LOG_INFO(Logger_) << "Ignore thin callback " << callbackName << " on thick client";
        return CreateIrrelevantResponseMusicNotFound();
    }
    if (IsIgnoredShotsFeedback(callback->GetPayload())) {
        LOG_INFO(Logger_) << "Ignore shot callback " << callbackName;
        return *TRunResponseBuilder::MakeIrrelevantResponse(Nlg_, "Мне нечего делать.", ConstructBodyRenderer(Request_));
    } else {
        THwFrameworkRunResponseBuilder response(Ctx_, &Nlg_, ConstructBodyRenderer(Request_));
        auto args = MakeMusicArguments(Logger_, Request_, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                             /* isNewContentRequestedByUser= */ false);
        response.CreateCommitCandidate(args);
        // TODO(jan-fazli): This might be the cause of losing ShouldListen
        return *std::move(response).BuildResponse();
    }
}

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::HandleRecoveryCallback() {
    return MakeThinClientDefaultResponseWithEmptyContinueArguments();
}

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::HandleOnboardingOnPlayDeclineCallback() {
    LOG_INFO(Logger_) << "Declined music play after music onboarding tracks game";
    return CreateOnboardingResponse(
        Ctx_, Request_, Nlg_, GetScenarioStateOrDummy(), "track__game_music_play_decline",
        TOnboardingResponseParamsBuilder{}
            .SetFillPlayerFeatures(false)
            .SetIntent(TString{MUSIC_ONBOARDING_TRACKS_FRAME})
            .Build()
    );
}

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::HandleOnboardingOnTracksGameDeclineCallback() {
    LOG_INFO(Logger_) << "Declined starting tracks game in music onboarding";
    return CreateOnboardingResponse(
        Ctx_, Request_, Nlg_, GetScenarioStateOrDummy(), "track__game_start_decline",
        TOnboardingResponseParamsBuilder{}
            .SetFillPlayerFeatures(false)
            .SetIntent(TString{MUSIC_ONBOARDING_TRACKS_FRAME})
            .Build()
    );
}

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::HandleOnboardingOnDontKnowCallback() {
    LOG_INFO(Logger_) << "Genre/artist not known by user in music onboarding";
    return CreateOnboardingResponse(
        Ctx_, Request_, Nlg_, GetScenarioStateOrDummy(), "asking__repeat",
        TOnboardingResponseParamsBuilder{}
            .SetFillPlayerFeatures(false)
            .SetIntent(TString{MUSIC_ONBOARDING_DONT_KNOW_FRAME})
            .SetTryReask(true)
            .SetStopOnNoReask(true)
            .Build()
    );
}

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::HandleOnboardingOnSilenceCallback() {
    LOG_INFO(Logger_) << "User silence in music onboarding";
    return CreateOnboardingResponse(
        Ctx_, Request_, Nlg_, GetScenarioStateOrDummy(), "asking__repeat",
        TOnboardingResponseParamsBuilder{}
            .SetFillPlayerFeatures(false)
            .SetIntent(TString{MUSIC_ONBOARDING_SILENCE_INTENT})
            .SetTryReask(true)
            .SetStopOnNoReask(true)
            .Build()
    );
}

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::HandleOnboardingOnRepeatedSkipDeclineCallback() {
    LOG_INFO(Logger_) << "Declined repeated skip proposal in music onboarding";
    return HandleMusicOnboardingRepeatedSkipDecline(Ctx_, Request_, Nlg_, GetScenarioStateOrDummy());
}

} // namespace NAlice::NHollywood::NMusic::NImpl
