#include "audio_player_commands.h"
#include "onboarding.h"

#include <alice/hollywood/library/capability_wrapper/capability_wrapper.h>
#include <alice/hollywood/library/framework/framework_migration.h>
#include <alice/hollywood/library/player_features/player_features.h>
#include <alice/hollywood/library/response/push.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/analytics_info/analytics_info.h>
#include <alice/hollywood/library/scenarios/music/biometry/process_biometry.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/consts.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/content_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/similar_radio_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/repeated_skip.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/result_renders.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/track_announce.h>
#include <alice/hollywood/library/scenarios/music/s3_animations/s3_animations.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/protos/data/scenario/music/content_info.pb.h>

#include <alice/library/music/defs.h>
#include <alice/library/proto/protobuf.h>
#include <alice/library/scled_animations/scled_animations_directive_hw.h>

#include <alice/bass/libs/client/experimental_flags.h>

#include <library/cpp/cgiparam/cgiparam.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf MUSIC_PLAYER_SCREEN = "music_player";
const i32 LIKE_DISLIKE_DELAY_SHORT_SEC = 60;
const i32 LIKE_DISLIKE_DELAY_LONG_SEC = 15 * LIKE_DISLIKE_DELAY_SHORT_SEC;

bool IsNeedGoBackToThinClient(TRTLogger& logger, const TScenarioRunRequestWrapper& request) {
    const auto& deviceState = request.BaseRequestProto().GetDeviceState();
    if (!deviceState.HasAudioPlayer()) {
        LOG_INFO(logger) << "Have no audio_player in device_state";
        return false;
    }
    if (!deviceState.HasMusic()) {
        LOG_INFO(logger) << "Have no music_player in device_state";
        return false;
    }
    if (!NMusic::IsMusicOwnerOfAudioPlayer(deviceState)) {
        LOG_INFO(logger) << "Music does not own audio_player";
        return false;
    }
    if (deviceState.GetMusic().GetPlayer().GetPause()) {
        LOG_INFO(logger) << "music_player is on pause";
        return false;
    }
    const auto& audioPlayer = deviceState.GetAudioPlayer();
    if (audioPlayer.GetPlayerState() != TDeviceState_TAudioPlayer_TPlayerState_Stopped) {
        LOG_INFO(logger) << "audio_player is not stopped";
        return false;
    }
    const auto nowTimestamp = request.BaseRequestProto().GetServerTimeMs();
    const auto lastStopTimestamp = audioPlayer.GetLastStopTimestamp();
    const auto diff = nowTimestamp - lastStopTimestamp;
    LOG_INFO(logger) << "Calculated diff=" << diff;
    return nowTimestamp - lastStopTimestamp < 30000;
}

bool IsPlayerCommandApplicable(const TScenarioRunRequestWrapper& request) {
    const auto& deviceState = request.BaseRequestProto().GetDeviceState();

    if (IsAudioPlayerPlaying(deviceState)) {
        return true;
    }

    const auto nowTime = TInstant::Seconds(request.ClientInfo().Epoch);
    const auto lastStopTime =  TInstant::MilliSeconds(deviceState.GetAudioPlayer().GetLastStopTimestamp());
    const ui64 diff = (nowTime - lastStopTime).Seconds();
    if (deviceState.GetIsTvPluggedIn()) {
        if (IsMusicOwnerOfAudioPlayer(deviceState) &&
            deviceState.GetVideo().GetCurrentScreen() == MUSIC_PLAYER_SCREEN)
        {
            return diff < LIKE_DISLIKE_DELAY_LONG_SEC;
        }
        return false;
    }

    return diff < LIKE_DISLIKE_DELAY_SHORT_SEC;
}

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerLikeDislikeCommandOnApply(TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg, const bool tracksGame, const bool isLike)
{
    auto& logger = ctx.Ctx.Logger();
    // For now likes on apply can only be in onboarding tracks game
    Y_ENSURE(!isLike || tracksGame);

    THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));

    auto args = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         IsNewContentRequestedByCommandByDefault(TMusicArguments_EPlayerCommand_Dislike));
                                         
    args.SetPlayerCommand(isLike ? TMusicArguments_EPlayerCommand_Like : TMusicArguments_EPlayerCommand_Dislike);
    args.SetIsShotPlaying(IsThinClientShotPlaying(request));
    args.SetOnboardingTracksGame(tracksGame);

    const auto biometryData = ProcessBiometryOrFallback(logger, request, GetUid(request));

    // We need biometryResult to render NLG
    auto& biometryResult = *args.MutableBiometryResult();
    biometryResult.SetIsGuestUser(biometryData.IsIncognitoUser);
    biometryResult.SetOwnerName(biometryData.OwnerName);

    response.SetApplyArguments(args);

    // NOTE: Analytics info is filled later, in the render apply stage, see
    // https://a.yandex-team.ru/arc_vcs/alice/hollywood/library/scenarios/music/music_backend_api/result_renders.cpp?rev=r8381716#L644
    FillPlayerFeatures(logger, request, response);
    return std::move(response).BuildResponse();
}

bool FrameHasSlotEqualsValue(const TFrame& frame, const TStringBuf slotName, const TStringBuf slotValue) {
    const auto slot = frame.FindSlot(slotName);
    if (slot && slot->Value.AsString() == slotValue) {
        return true;
    }
    return false;
}

bool ShouldFallback(const TMaybe<TScenarioState>& scenarioState) {
    if (!scenarioState || !scenarioState->HasQueue()) {
        return true;
    }

    const auto& contentId = scenarioState->GetQueue().GetPlaybackContext().GetContentId();

    if (contentId.GetId().empty() && contentId.GetIds().empty()) {
        return true;
    }

    return false;
}

} // namespace

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerLikeCommand(TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg, const bool tracksGame)
{
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Handling Like player command...";

    if (!IsPlayerCommandApplicable(request)) {
        return NImpl::RunThinClientRenderUnknownMusicPlayerCommand(
            ctx, request, nlg, TEMPLATE_PLAYER_LIKE);
    }

    if (tracksGame) {
        return HandlePlayerLikeDislikeCommandOnApply(ctx, request, nlg, tracksGame, /* isLike= */ true);
    }

    TScenarioState scState;
    auto haveScState = ReadScenarioState(request.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(logger, scState);
    Y_ENSURE(haveScState);
    TMusicQueueWrapper mq(logger, *scState.MutableQueue());
    if (mq.IsFmRadio()) {
        return NImpl::RunThinClientRenderFmRadioUnsupportedPlayerCommand(
            mq, scState, ctx, request, nlg, TEMPLATE_PLAYER_LIKE, TMusicArguments_EPlayerCommand_Like);
    }

    const auto biometryData = ProcessBiometryOrFallback(logger, request, GetUid(request));
    THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));

    const bool isShotPlaying = IsThinClientShotPlaying(request);
    if (!biometryData.IsIncognitoUser) {
        auto args = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                                        IsNewContentRequestedByCommandByDefault(TMusicArguments_EPlayerCommand_Like));
        args.SetPlayerCommand(TMusicArguments_EPlayerCommand_Like);
        args.SetIsShotPlaying(isShotPlaying);
        // No need to care about args.BiometryResult in this case. NLG already rendered.
        // 'Like' handle will use uid from BlackBox or from GuestOptions DataSource if present
        response.CreateCommitCandidate(args);
    }

    if (isShotPlaying) {
        NImpl::RenderShotsLikeDislikeFeedback(ctx, request, biometryData, response, /* isLike = */ true);
    } else {
        NImpl::RenderLikeNlgResult(ctx, request, biometryData, response);
    }
    auto& bodyBuilder = *response.GetResponseBodyBuilder();
    if (TScenarioState scState; ReadScenarioState(request.BaseRequestProto(), scState)) {
        TryInitPlaybackContextBiometryOptions(logger, scState);
        TRepeatedSkip{scState, logger}.ResetCount();
        bodyBuilder.SetState(scState);
        CreateAndFillAnalyticsInfoForPlayerCommand(TMusicArguments_EPlayerCommand_Like, bodyBuilder,
                                                   request.ServerTimeMs(),
                                                   scState.GetProductScenarioName());
        const TCapabilityWrapper<TScenarioRunRequestWrapper> capabilityWrapper(
            request,
            GetEnvironmentStateProto(request)
        );
        if (capabilityWrapper.HasLedDisplay()) {
            bodyBuilder.AddDirective(BuildDrawLedScreenDirective(GetFrontalLedImage(TMusicArguments_EPlayerCommand_Like)));
        } else if (capabilityWrapper.HasScledDisplay()) {
            NScledAnimation::AddStandardScled(bodyBuilder, NScledAnimation::EScledAnimations::SCLED_ANIMATION_LIKE);
            bodyBuilder.AddTtsPlayPlaceholderDirective();
        } else if (capabilityWrapper.SupportsS3Animations()) {
            const auto animationPath = TryGetS3AnimationPath(TMusicArguments_EPlayerCommand_Like);
            if (animationPath.Defined()) {
                bodyBuilder.AddDirective(BuildDrawAnimationDirective(*animationPath));
                bodyBuilder.AddTtsPlayPlaceholderDirective();
            }
        }
    } else {
        CreateAndFillAnalyticsInfoForPlayerCommand(TMusicArguments_EPlayerCommand_Like, bodyBuilder,
                                                   request.ServerTimeMs());
    }
    FillPlayerFeatures(logger, request, response);
    return std::move(response).BuildResponse();
}

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerDislikeCommand(TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg, const bool tracksGame)
{
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Handling Dislike player command...";

    if (!IsPlayerCommandApplicable(request)) {
        return NImpl::RunThinClientRenderUnknownMusicPlayerCommand(
            ctx, request, nlg, TEMPLATE_PLAYER_DISLIKE);
    }

    TScenarioState scState;
    auto haveScState = ReadScenarioState(request.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(logger, scState);
    Y_ENSURE(haveScState);
    TMusicQueueWrapper mq(logger, *scState.MutableQueue());
    if (mq.IsFmRadio()) {
        return NImpl::RunThinClientRenderFmRadioUnsupportedPlayerCommand(
            mq, scState, ctx, request, nlg, TEMPLATE_PLAYER_DISLIKE, TMusicArguments_EPlayerCommand_Dislike);
    }

    return HandlePlayerLikeDislikeCommandOnApply(ctx, request, nlg, tracksGame, /* isLike = */ false);
}

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerShuffleCommand(TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg)
{
    auto& logger = ctx.Ctx.Logger();

    if (!IsPlayerCommandApplicable(request)) {
        return NImpl::RunThinClientRenderUnknownMusicPlayerCommand(
            ctx, request, nlg, TEMPLATE_PLAYER_SHUFFLE);
    }

    TScenarioState scState;
    const bool haveScState = ReadScenarioState(request.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(logger, scState);
    Y_ENSURE(haveScState);
    TMusicQueueWrapper mq(logger, *scState.MutableQueue());

    if (mq.IsGenerative()) {
        return NImpl::RunThinClientRenderGenerativeUnsupportedPlayerCommand(
            scState, ctx, request, nlg, TEMPLATE_PLAYER_SHUFFLE, TMusicArguments_EPlayerCommand_Shuffle);
    } else if (mq.IsFmRadio()) {
        return NImpl::RunThinClientRenderFmRadioUnsupportedPlayerCommand(
            mq, scState, ctx, request, nlg, TEMPLATE_PLAYER_SHUFFLE, TMusicArguments_EPlayerCommand_Shuffle);
    } else if (mq.IsRadio()) {
        LOG_INFO(logger) << "Handling radio Shuffle player command...";
        THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));

        NImpl::RenderShuffleNlgResult(ctx, request, response,
                                              /* isRadio = */ true);
        auto& bodyBuilder = *response.GetResponseBodyBuilder();
        bodyBuilder.SetState(scState);

        CreateAndFillAnalyticsInfoForPlayerCommand(TMusicArguments_EPlayerCommand_Shuffle, bodyBuilder,
                                                   request.ServerTimeMs(),
                                                   scState.GetProductScenarioName());
        FillPlayerFeatures(logger, request, response);
        return std::move(response).BuildResponse();
    }

    // Common handling for music Shuffle command
    return HandlePlayerCommands(ctx, request, nlg, TMusicArguments_EPlayerCommand_Shuffle);
}

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerUnshuffleCommand(TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg)
{
    auto& logger = ctx.Ctx.Logger();
    TScenarioState scState;
    const bool haveScState = ReadScenarioState(request.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(logger, scState);
    Y_ENSURE(haveScState);
    TMusicQueueWrapper mq(logger, *scState.MutableQueue());

    if (mq.IsGenerative()) {
        return NImpl::RunThinClientRenderGenerativeUnsupportedPlayerCommand(
            scState, ctx, request, nlg, TEMPLATE_PLAYER_SHUFFLE, TMusicArguments_EPlayerCommand_Unshuffle);
    } else if (mq.IsRadio()) {
        LOG_INFO(logger) << "Handling radio Unshuffle player command...";
        THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));

        NImpl::RenderShuffleNlgResult(ctx, request, response, /* isRadio = */ true);
        auto& bodyBuilder = *response.GetResponseBodyBuilder();
        bodyBuilder.SetState(scState);

        CreateAndFillAnalyticsInfoForPlayerCommand(TMusicArguments_EPlayerCommand_Unshuffle, bodyBuilder,
                                                   request.ServerTimeMs(), scState.GetProductScenarioName());
        FillPlayerFeatures(logger, request, response);
        return std::move(response).BuildResponse();
    }

    LOG_INFO(logger) << "Handling Unshuffle player command...";
    THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));
    auto args = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         IsNewContentRequestedByCommandByDefault(TMusicArguments_EPlayerCommand_Unshuffle));

    auto& searchResult = *args.MutableMusicSearchResult();
    searchResult.SetContentId(mq.ContentId().GetId());
    searchResult.SetContentType(ContentTypeToText(mq.ContentId().GetType()));

    auto& playbackOptions = *args.MutablePlaybackOptions();
    const auto& playbackContext = mq.GetPlaybackContext();
    playbackOptions.SetShuffle(false);
    playbackOptions.SetRepeatType(playbackContext.GetRepeatType());
    playbackOptions.SetDisableAutoflow(playbackContext.GetDisableAutoflow());
    playbackOptions.SetPlaySingleTrack(playbackContext.GetPlaySingleTrack());
    playbackOptions.SetDisableNlg(playbackContext.GetDisableNlg());
    Y_ENSURE(mq.HasCurrentItem());
    playbackOptions.SetStartFromTrackId(mq.CurrentItem().GetTrackId());
    playbackOptions.SetFrom(playbackContext.GetFrom());
    playbackOptions.SetDisableHistory(playbackContext.GetDisableHistory());
    *playbackOptions.MutableContentInfo() = playbackContext.GetContentInfo();

    args.SetPlayerCommand(TMusicArguments_EPlayerCommand_Unshuffle);
    args.SetIsShotPlaying(IsThinClientShotPlaying(request));
    response.SetContinueArguments(args);

    FillPlayerFeatures(logger, request, response);
    return std::move(response).BuildResponse();
}

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerPrevTrackCommand(TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg)
{
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Handling PrevTrack player command...";

    if (const auto frameProto = request.Input().FindSemanticFrame(NAlice::NMusic::PLAYER_PREV_TRACK)) {
        const auto frame = TFrame::FromProto(*frameProto);
        if (FrameHasSlotEqualsValue(frame, NAlice::NMusic::SLOT_PLAYER_TYPE, TStringBuf("video"))) {
            LOG_INFO(logger) << "Irrelevant because slot player_type == video";
            return TRunResponseBuilder::MakeIrrelevantResponse(nlg, "Не могу включить предыдущее видео.", ConstructBodyRenderer(request));
        }
    }

    TScenarioState scState;
    auto haveScState = ReadScenarioState(request.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(logger, scState);
    Y_ENSURE(haveScState);

    // TODO(vitvlkv): Remove this hack after HOLLYWOOD-190 will go to prod
    if (IsNeedGoBackToThinClient(logger, request)) {
        LOG_INFO(logger) << "Falling back to ThinClient music...";
        THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));
        auto args = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         IsNewContentRequestedByCommandByDefault(TMusicArguments_EPlayerCommand_PrevTrack));
        args.SetPlayerCommand(TMusicArguments_EPlayerCommand_Replay); // We use Replay, because "previous track" is
                                                                      // actually still the current one
        response.SetContinueArguments(args);

        // Now we calculate PlayerFeatures in an unfair way. Because it's a hack!
        NScenarios::TScenarioRunResponse_TFeatures_TPlayerFeatures playerFeatures;
        playerFeatures.SetRestorePlayer(true);
        response.AddPlayerFeatures(std::move(playerFeatures));

        return std::move(response).BuildResponse();
    }

    TMusicQueueWrapper mq(logger, *scState.MutableQueue());
    if (!mq.HasPreviousItem()) {
        THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));
        if (mq.IsHistoryDisabled()) {
            response.SetIrrelevant();
        }

        NImpl::RenderNoPrevTrackNlgResult(ctx, request, response);
        auto& bodyBuilder = *response.GetResponseBodyBuilder();
        if (mq.GetRepeatType() == RepeatTrack) {
            mq.RepeatPlayback(RepeatNone);
        }
        bodyBuilder.SetState(scState);

        FillPlayerFeatures(logger, request, response);
        return std::move(response).BuildResponse();
    }

    // Common handling for music PrevTrack command
    return HandlePlayerCommands(ctx, request, nlg, TMusicArguments_EPlayerCommand_PrevTrack);
}

std::unique_ptr<NScenarios::TScenarioRunResponse> HandleTimestampSkipGenerativeStreamCommand(TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg)
{
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Handling TimestampSkip player command...";
    THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));
    auto args = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         /* isNewContentRequestedByUser= */ false);
    args.SetPlayerCommand(TMusicArguments_EPlayerCommand_NextTrack);

    response.SetApplyArguments(args);

    FillPlayerFeatures(logger, request, response);
    return std::move(response).BuildResponse();
}

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerNextTrackCommand(TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg)
{
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Handling NextTrack player command...";

    if (const auto frameProto = request.Input().FindSemanticFrame(NAlice::NMusic::PLAYER_NEXT_TRACK)) {
        const auto frame = TFrame::FromProto(*frameProto);
        if (FrameHasSlotEqualsValue(frame, NAlice::NMusic::SLOT_PLAYER_TYPE, TStringBuf("video"))) {
            LOG_INFO(logger) << "Irrelevant because slot player_type == video";
            return TRunResponseBuilder::MakeIrrelevantResponse(nlg, "Не могу включить следующее видео.", ConstructBodyRenderer(request));
        }
    }

    TScenarioState scState;
    auto haveScState = ReadScenarioState(request.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(logger, scState);
    Y_ENSURE(haveScState);

    TMusicQueueWrapper mq(logger, *scState.MutableQueue());

    if (!mq.IsCurrentTrackLast() || mq.GetRepeatType() == RepeatAll) {
        return HandlePlayerCommands(ctx, request, nlg, TMusicArguments_EPlayerCommand_NextTrack);
    } else if (mq.IsGenerative()) {
        return HandleTimestampSkipGenerativeStreamCommand(ctx, request, nlg);
    }

    THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));
    if (!mq.IsAutoflowDisabled()) {
        // We should turn on the radio. Set ContinueArgs for radio
        // XXX(sparkle): move this branch to function if needed, don't copy-paste
        const auto& contentId = mq.ContentId();
        const auto objType = ContentTypeToText(contentId.GetType());
        auto objId = contentId.GetId();

        if (objType == "playlist") {
            auto playlistId = TPlaylistId::FromString(objId);
            Y_ENSURE(playlistId, "Failed to create playlistId from " << objId);
            objId = playlistId->ToStringForRadio();
        }
        const auto radioStationId = SimilarRadioId(mq.ContentId());

        auto args = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         IsNewContentRequestedByCommandByDefault(TMusicArguments_EPlayerCommand_NextTrack));
        args.MutableRadioRequest()->AddStationIds(radioStationId);
        args.MutablePlaybackOptions()->SetDisableNlg(true);
        args.MutableAnalyticsInfoData()->SetPlayerCommand(TMusicArguments_EPlayerCommand_NextTrack);

        const TStringBuf uid = GetUid(request);
        args.MutableAccountStatus()->SetUid(uid.data(), uid.size());

        response.SetContinueArguments(args);
    } else {
        auto& bodyBuilder = response.CreateResponseBodyBuilder();
        scState.MutableQueue()->MutablePlaybackContext()->SetRepeatType(RepeatNone);
        bodyBuilder.SetState(scState);
        response.SetIrrelevant();
    }
    FillPlayerFeatures(logger, request, response);
    return std::move(response).BuildResponse();
}

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerChangeTrackNumberCommand(TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg) {
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Handling ChangeTrackNumber player command...";

    TMaybe<size_t> trackNumber = Nothing();
    if (const auto frameProto = request.Input().FindSemanticFrame(NAlice::NMusic::MUSIC_PLAYER_CHANGE_TRACK_NUMBER)) {
        const auto frame = TFrame::FromProto(*frameProto);
        if (FrameHasSlotEqualsValue(frame, NAlice::NMusic::SLOT_OFFSET, TStringBuf("newest"))) {
            trackNumber = 0;
        }
    }

    TScenarioState scState;
    auto haveScState = ReadScenarioState(request.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(logger, scState);
    Y_ENSURE(haveScState);

    TMusicQueueWrapper mq(logger, *scState.MutableQueue());
    Y_ENSURE(mq.HasCurrentItem());
    if (!mq.IsPaged() || !trackNumber.Defined() || !mq.CurrentItem().GetRememberPosition()) {
        return TRunResponseBuilder::MakeIrrelevantResponse(nlg, "Не могу это сделать.", ConstructBodyRenderer(request));
    }

    THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));
    auto args = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         IsNewContentRequestedByCommandByDefault(TMusicArguments_EPlayerCommand_ChangeTrackNumber));
    args.SetPlayerCommand(TMusicArguments_EPlayerCommand_ChangeTrackNumber);
    auto& playbackOptions = *args.MutablePlaybackOptions();
    playbackOptions.SetTrackOffsetIndex(*trackNumber);
    response.SetContinueArguments(args);

    FillPlayerFeatures(logger, request, response);
    return std::move(response).BuildResponse();
}

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerChangeTrackVersionCommand(
    TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg)
{
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Handling ChangeTrackVersion player command...";

    TScenarioState scState;
    auto haveScState = ReadScenarioState(request.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(logger, scState);
    Y_ENSURE(haveScState);

    TMusicQueueWrapper mq(logger, *scState.MutableQueue());
    if (!mq.HasCurrentItem()) {
        return TRunResponseBuilder::MakeIrrelevantResponse(nlg, "Не могу это сделать.", ConstructBodyRenderer(request));
    }

    THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));
    auto args = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         IsNewContentRequestedByCommandByDefault(TMusicArguments_EPlayerCommand_ChangeTrackVersion));
    args.SetPlayerCommand(TMusicArguments_EPlayerCommand_ChangeTrackVersion);
    if (const auto frameProto = request.Input().FindSemanticFrame(NAlice::NMusic::MUSIC_PLAYER_CHANGE_TRACK_VERSION)) {
        const auto frame = TFrame::FromProto(*frameProto);
        if (const auto slot = frame.FindSlot(NAlice::NMusic::SLOT_TRACK_VERSION); slot) {
            auto& playbackOptions = *args.MutablePlaybackOptions();
            playbackOptions.SetTrackVersion(slot->Value.AsString());
        }
    }
    response.SetContinueArguments(args);

    FillPlayerFeatures(logger, request, response);
    return std::move(response).BuildResponse();
}

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerContinueCommand(TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg) {
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Handling Continue player command...";

    if (const auto frameProto = request.Input().FindSemanticFrame(NAlice::NMusic::PLAYER_CONTINUE)) {
        const auto frame = TFrame::FromProto(*frameProto);
        if (FrameHasSlotEqualsValue(frame, NAlice::NMusic::SLOT_PLAYER_TYPE, TStringBuf("video"))) {
            LOG_INFO(logger) << "Irrelevant because slot player_type == video";
            return TRunResponseBuilder::MakeIrrelevantResponse(nlg, "Не могу продолжить видео.", ConstructBodyRenderer(request));
        }
        if (FrameHasSlotEqualsValue(frame, NAlice::NMusic::SLOT_PLAYER_ACTION_TYPE, TStringBuf("watch"))) {
            LOG_INFO(logger) << "Irrelevant because slot player_action_type == watch";
            return TRunResponseBuilder::MakeIrrelevantResponse(nlg, "Не могу продолжить видео.", ConstructBodyRenderer(request));
        }
    }

    TScenarioState scState;
    auto haveScState = ReadScenarioState(request.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(logger, scState);
    Y_ENSURE(haveScState);

    if (const auto& deviceState = request.BaseRequestProto().GetDeviceState();
        IsMusicOwnerOfAudioPlayer(deviceState) &&
        deviceState.GetAudioPlayer().GetPlayerState() == TDeviceState_TAudioPlayer_TPlayerState_Playing) {
        LOG_INFO(logger) << "AudioPlayer is already in Playing state, nothing to do...";
        THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));
        auto& bodyBuilder = response.CreateResponseBodyBuilder();
        TNlgData nlgData{logger, request};
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_PLAYER_CONTINUE, "render_result",
                                                       /* buttons = */ {}, nlgData);
        bodyBuilder.SetState(scState);
        CreateAndFillAnalyticsInfoForPlayerCommand(TMusicArguments_EPlayerCommand_Continue, bodyBuilder,
                                                   request.ServerTimeMs(), scState.GetProductScenarioName());
        FillPlayerFeatures(logger, request, response);
        return std::move(response).BuildResponse();
    }

    TMusicQueueWrapper mq(logger, *scState.MutableQueue());
    if (mq.IsGenerative()) {
        LOG_INFO(logger) << "Handling Continue player command for generative stream...";
        THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));
        auto args = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         /* isNewContentRequestedByUser= */ false);
        args.SetPlayerCommand(TMusicArguments_EPlayerCommand_Continue);
        response.SetApplyArguments(args);
        FillPlayerFeatures(logger, request, response);
        return std::move(response).BuildResponse();
    }

    // Common handling for music Continue command
    return HandlePlayerCommands(ctx, request, nlg, TMusicArguments_EPlayerCommand_Continue);
}

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerReplayCommand(TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg)
{
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Handling Replay player command...";

    TScenarioState scState;
    const bool haveScState = ReadScenarioState(request.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(logger, scState);
    Y_ENSURE(haveScState);
    TMusicQueueWrapper mq(logger, *scState.MutableQueue());

    if (mq.IsGenerative()) {
        return NImpl::RunThinClientRenderGenerativeUnsupportedPlayerCommand(
            scState, ctx, request, nlg, TEMPLATE_PLAYER_REPLAY, TMusicArguments_EPlayerCommand_Replay);
    } else if (mq.IsFmRadio()) {
        return NImpl::RunThinClientRenderFmRadioUnsupportedPlayerCommand(
            mq, scState, ctx, request, nlg, TEMPLATE_PLAYER_REPLAY, TMusicArguments_EPlayerCommand_Replay);
    }

    // Common handling for music Replay command
    return HandlePlayerCommands(ctx, request, nlg, TMusicArguments_EPlayerCommand_Replay);
}

std::unique_ptr<NScenarios::TScenarioRunResponse> HandlePlayerCommands(TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg, TMusicArguments_EPlayerCommand playerCommand)
{
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Handling " << TMusicArguments_EPlayerCommand_Name(playerCommand) << " player command...";
    THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));
    auto args = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         IsNewContentRequestedByCommandByDefault(playerCommand));
    args.SetPlayerCommand(playerCommand);
    args.SetIsShotPlaying(IsThinClientShotPlaying(request));
    response.SetContinueArguments(args);

    FillPlayerFeatures(logger, request, response);
    return std::move(response).BuildResponse();
}

std::unique_ptr<NScenarios::TScenarioRunResponse> HandleThinClientPlayerCommand(TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request,
    TMusicArguments::EPlayerCommand playerCommand,
    TNlgWrapper& nlg,
    const TStringBuf playerFrameName,
    TMaybe<TScenarioState>& scenarioState)
{
    auto& logger = ctx.Ctx.Logger();
    Y_ENSURE(playerCommand != TMusicArguments_EPlayerCommand_None);

    if (ShouldFallback(scenarioState)) {
        LOG_INFO(logger) << "Failed to get TScenarioState with TMusicQueue from scenario state. PlayerCommand "
                            << TMusicArguments_EPlayerCommand_Name(playerCommand)
                            << " cannot proceed without it."
                            << " Falling back to user radio station aka 'включи музыку'...";
        THwFrameworkRunResponseBuilder response{ctx, &nlg, ConstructBodyRenderer(request)};
        if (IsThinRadioSupported(request)) {
            auto args = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         /* isNewContentRequestedByUser= */ true);    // NOTE(klim-roma): as we falling back to start radio, it appears to be a request for new content
            args.MutableRadioRequest()->AddStationIds(USER_RADIO_STATION_ID);
            args.MutablePlaybackOptions()->SetDisableNlg(true);
            args.MutableAnalyticsInfoData()->SetPlayerCommand(playerCommand);
            response.SetContinueArguments(args);
            LOG_INFO(logger) << "Turning on ThinClient radio, stationId=" << USER_RADIO_STATION_ID;
        } else {
            auto args = MakeMusicArgumentsImpl(TMusicArgumentsParams{
                TMusicArguments_EExecutionFlowType_BassRadio,
                /* .IsNewContentRequestedByUser= */ true
            }); // don't pass datasources

            args.MutableAnalyticsInfoData()->SetPlayerCommand(playerCommand);
            response.SetContinueArguments(args);
            LOG_INFO(logger) << "Turning on BASS radio, user's personal station";
        }

        response.SetFeaturesIntent(TString{playerFrameName});
        FillPlayerFeatures(logger, request, response);
        return std::move(response).BuildResponse();
    }
    const bool tracksGame = InTracksGame(logger, request.BaseRequestProto().GetDeviceState(), *scenarioState);
    switch (playerCommand) {
        case TMusicArguments_EPlayerCommand_Like:
            return HandlePlayerLikeCommand(ctx, request, nlg, tracksGame);
        case TMusicArguments_EPlayerCommand_Dislike:
            return HandlePlayerDislikeCommand(ctx, request, nlg, tracksGame);
        case TMusicArguments_EPlayerCommand_PrevTrack:
            return HandlePlayerPrevTrackCommand(ctx, request, nlg);
        case TMusicArguments_EPlayerCommand_NextTrack:
            return HandlePlayerNextTrackCommand(ctx, request, nlg);
        case TMusicArguments_EPlayerCommand_Shuffle:
            return HandlePlayerShuffleCommand(ctx, request, nlg);
        case TMusicArguments_EPlayerCommand_Unshuffle:
            return HandlePlayerUnshuffleCommand(ctx, request, nlg);
        case TMusicArguments_EPlayerCommand_ChangeTrackNumber:
            return HandlePlayerChangeTrackNumberCommand(ctx, request, nlg);
        case TMusicArguments_EPlayerCommand_ChangeTrackVersion:
            return HandlePlayerChangeTrackVersionCommand(ctx, request, nlg);
        case TMusicArguments_EPlayerCommand_Continue:
            return HandlePlayerContinueCommand(ctx, request, nlg);
        case TMusicArguments_EPlayerCommand_Replay:
            return HandlePlayerReplayCommand(ctx, request, nlg);
        default:
            return HandlePlayerCommands(ctx, request, nlg, playerCommand);
    }
}

} // NAlice::NHollywood::NMusic
