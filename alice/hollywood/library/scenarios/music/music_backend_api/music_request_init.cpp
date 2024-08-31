#include "music_request_init.h"
#include "music_common.h"
#include "entity_search_response_parsers.h"
#include "find_track_idx_response_parsers.h"
#include "repeated_skip.h"

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/music/biometry/process_biometry.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/consts.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/content_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/similar_radio_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_requests/content_requests.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/like_dislike_handlers.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/report_handlers.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/shots.h>
#include <alice/hollywood/library/scenarios/music/proto/callback_payload.pb.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/billing/billing.h>
#include <alice/library/cachalot_cache/cachalot_cache.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/music/defs.h>

#include <alice/protos/data/scenario/music/content_info.pb.h>

#include <util/string/join.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf REQUEST_META_ITEM = "mm_scenario_request_meta";
const TString CACHE_LOOKUP_NODE_NAME = "MUSIC_SCENARIO_CACHE_LOOKUP";
const TString CONTENT_PROXY_CACHE_GET_REQUEST_ITEM = "content_proxy_cache_get_request";

constexpr size_t RADIO_BATCH_SIZE = 5;

const THashSet<TContentId_EContentType> NEED_SIMILAR_ACCEPTED_TYPES = {
    TContentId_EContentType_Track,
    TContentId_EContentType_Album,
    TContentId_EContentType_Artist,
    TContentId_EContentType_Playlist
};

void ForwardRequestAndMeta(NAppHost::IServiceContext& ctx, const NScenarios::TScenarioApplyRequest& request) {
    ctx.AddProtobufItem(ctx.GetOnlyProtobufItem<NScenarios::TRequestMeta>(REQUEST_META_ITEM), REQUEST_META_ITEM);
    ctx.AddProtobufItem(request, REQUEST_ITEM);
}

bool IsNextTrackCallback(const NScenarios::TCallbackDirective* callback) {
    return callback && callback->GetName() == MUSIC_THIN_CLIENT_NEXT_CALLBACK;
}

bool IsRecoveryCallback(const NScenarios::TCallbackDirective* callback) {
    return callback && callback->GetName() == MUSIC_THIN_CLIENT_RECOVERY_CALLBACK;
}

void UpdateCurrentTrackOffsetMs(const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                TMusicContext& mCtx) {
    const auto& deviceState = applyRequest.BaseRequestProto().GetDeviceState();
    i32 offsetMs = 0;
    if (IsMusicOwnerOfAudioPlayer(deviceState)) {
        offsetMs = deviceState.GetAudioPlayer().GetOffsetMs();
    }
    mCtx.SetOffsetMs(offsetMs);
}

void AddNoveltyAlbumSearchFlagIfNeeded(TRTLogger& logger, TContentId contentId,
                                       const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                       TMusicContext& mCtx, NAppHost::IServiceContext& serviceCtx)
{
    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();

    // NOTE: It is normal to have here other types, e.g. Playlist
    const auto isOnDemandType = contentId.GetType() == TContentId_EContentType_Track ||
                                contentId.GetType() == TContentId_EContentType_Album ||
                                contentId.GetType() == TContentId_EContentType_Artist;
    const bool hasNonEmptyArtistId = applyArgs.HasOnDemandRequest() &&
                                        !applyArgs.GetOnDemandRequest().GetArtistId().empty();
    if (isOnDemandType && hasNonEmptyArtistId) {
        *mCtx.MutableOnDemandRequest() = applyArgs.GetOnDemandRequest();
        if (const auto frameProto = applyRequest.Input().FindSemanticFrame(MUSIC_PLAY_FRAME)) {
            const auto musicPlayFrame = TFrame::FromProto(*frameProto);
            if (const auto noveltySlot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_NOVELTY)) {
                AddNoveltyAlbumSearchFlag(serviceCtx);
                LOG_INFO(logger) << "Added need_novelty_album_search flag";
            }
        }
    } else {
        LOG_INFO(logger) << "Skipping add need_novelty_album_search flag, because isOnDemandType="
                         << isOnDemandType << ", hasNonEmptyArtistId=" << hasNonEmptyArtistId;
    }
}

// TODO: Remove this ugly hack, after the prefetch bug will be fixed on the client side
// See https://st.yandex-team.ru/EXPERIMENTS-76986#613dd5953f687d71bbe735c0
ETrackChangeResult TryFixPrefetchBugIfNeeded(TRTLogger& logger, const TScenarioApplyRequestWrapper& applyRequest,
                                             TMusicQueueWrapper& mq, const ETrackChangeResult prevResult) {
    const auto& audioPlayer = applyRequest.BaseRequestProto().GetDeviceState().GetAudioPlayer();
    if (applyRequest.HasExpFlag(EXP_HW_MUSIC_ENABLE_PREFETCH_GET_NEXT_CORRECTION) &&
        prevResult == ETrackChangeResult::TrackChanged &&
        audioPlayer.HasCurrentlyPlaying() && mq.HasCurrentItem() &&
        audioPlayer.GetCurrentlyPlaying().GetStreamId() == mq.CurrentItem().GetTrackId()) {
        LOG_INFO(logger) << "Detected that ScenarioState and DeviceState are out of sync with each other"
                         << " (ScenarioState's next track is already the current track in DeviceState): "
                         << " DeviceState.trackId=" << audioPlayer.GetCurrentlyPlaying().GetStreamId()
                         << " DeviceState.trackTitle=" << audioPlayer.GetCurrentlyPlaying().GetTitle()
                         << " mq.ToStringDebug()=" << mq.ToStringDebug()
                         << " Will call one more mq.ChangeToNextTrack() to fix this problem";
        return mq.ChangeToNextTrack();
    } else {
        return prevResult;
    }
}


void TryUpdateContentIdIfNeedSimilar(TRTLogger& logger, const TScenarioApplyRequestWrapper& applyRequest,
                                     TMusicQueueWrapper& mq) {
    const auto isNeedSimilarAccepted = NEED_SIMILAR_ACCEPTED_TYPES.contains(mq.ContentId().GetType());

    if (const auto frameProto = applyRequest.Input().FindSemanticFrame(MUSIC_PLAY_FRAME); isNeedSimilarAccepted && frameProto) {
        const auto musicPlayFrame = TFrame::FromProto(*frameProto);
        if (const auto needSimilarSlot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_NEED_SIMILAR)) {
            if (needSimilarSlot->Value.AsString() == "need_similar") {
                TContentId radioContentId;
                radioContentId.SetType(TContentId_EContentType_Radio);
                radioContentId.AddIds(SimilarRadioId(mq.ContentId()));
                LOG_INFO(logger) << "Applying need_similar slot for content id " << radioContentId.GetIds()[0];
                mq.SetContentId(radioContentId);
            }
        }
    }
}

bool ShouldUseResumeFrom(
    const TScenarioApplyRequestWrapper& applyRequest,
    const TMusicArguments& applyArgs,
    const TMaybe<TContentId>& contentId)
{
    if (const auto frameProto = applyRequest.Input().FindSemanticFrame(MUSIC_PLAY_FRAME)) {
        const auto musicPlayFrame = TFrame::FromProto(*frameProto);
        if (const auto offsetSlot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_OFFSET)) {
            if (offsetSlot->Value.AsString() == "beginning") {
                return false;
            }
        }
    }

    if (applyArgs.GetPlaybackOptions().GetOffset() == "saved_progress") {
        return true;
    } else if (SAVE_PROGRESS_ALBUM_TYPES.contains(applyArgs.GetMusicSearchResult().GetSubtype()) &&
        contentId->GetType() == TContentId_EContentType_Album)
    {
        return true;
    } else if (applyRequest.HasExpFlag(TString::Join(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_USE_SAVE_PROGRESS,
                                                     '_',
                                                     applyArgs.GetMusicSearchResult().GetSubtype())) &&
        contentId->GetType() == TContentId_EContentType_Album)
    {
        return true;
    }

    return false;
}

void AddGenerativeFeedbackProxyRequest(TScenarioHandleContext& ctx, const NHollywood::TScenarioApplyRequestWrapper& request,
                                       const TMusicArguments& applyArgs, const TScenarioState& scState,
                                       const TMusicQueueWrapper& mq, TRTLogger& logger, const TString& type)
{
    auto isClientBiometryModeApplyRequest = IsClientBiometryModeApplyRequest(logger, applyArgs);
    auto musicRequestModeInfo = MakeMusicRequestModeInfoFromMusicArgs(applyArgs, scState, EAuthMethod::OAuth, isClientBiometryModeApplyRequest);
    auto metaProvider = MakeRequestMetaProviderFromMusicArgs(ctx.RequestMeta, applyArgs, isClientBiometryModeApplyRequest);
    auto req = MakeTimestampGenerativeFeedbackProxyRequest(
        mq, metaProvider, request.ClientInfo(), logger, request, type, musicRequestModeInfo);
    AddMusicProxyRequest(ctx, req.first, req.second);
}

void SetUpPlaybackMode(TRTLogger& logger, TScenarioState& scState, TMusicQueueWrapper& mq,
                       const NHollywood::TScenarioApplyRequestWrapper& request, const TMusicArguments& applyArgs)
{
    if (SupportsClientBiometry(request)) {
        auto isClientBiometryModeApplyRequest = IsClientBiometryModeApplyRequest(logger, applyArgs);
        SetUpPlaybackModeUsingClientBiometryDeprecated(logger, scState, request, applyArgs, isClientBiometryModeApplyRequest);
        mq.SetUpPlaybackModeUsingClientBiometry(request, applyArgs, isClientBiometryModeApplyRequest);
    } else {
        SetUpPlaybackModeUsingServerBiometryDeprecated(logger, scState, request, applyArgs);
        mq.SetUpPlaybackModeUsingServerBiometry(request, applyArgs);
    }
}

} // namespace

void AddMusicContentRequest(TScenarioHandleContext& ctx, const NHollywood::TScenarioApplyRequestWrapper& applyRequest) {
    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();
    auto& logger = ctx.Ctx.Logger();

    TMusicContext mCtx;
    auto& scState = *mCtx.MutableScenarioState();
    const bool haveScState = ReadScenarioState(applyRequest.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(logger, scState);
    TMusicQueueWrapper mq(logger, *scState.MutableQueue());

    if (!applyArgs.HasAccountStatus()) {
        LOG_ERR(logger) << "Failed to get AccountStatus from apply args";
        return;
    }

    if (applyArgs.GetIsNewContentRequestedByUser()) {
        SetUpPlaybackMode(logger, scState, mq, applyRequest, applyArgs);
    }

    *mCtx.MutableAccountStatus() = applyArgs.GetAccountStatus();
    if (!mCtx.GetAccountStatus().GetHasMusicSubscription()) {
        // We should clear any old Queue state that could came with the request
        LOG_INFO(logger) << "Clearing queue state, because user does not have subscription";
        scState.ClearQueue();
        scState.ClearBiometryUserId();
    }

    const auto playerCommand = applyArgs.GetPlayerCommand();
    const auto callback = applyRequest.Input().GetCallback();
    LOG_INFO(logger) << "AddMusicContentRequest: player command " << TMusicArguments_EPlayerCommand_Name(playerCommand)
                     << (callback ? " callback " + callback->GetName() : "");
    // Change to next track if we have got music_thin_client_turn_on_radio callback here.
    // Workaround https://st.yandex-team.ru/HOLLYWOOD-496 before https://st.yandex-team.ru/HOLLYWOOD-553 is implemented

    if (IsNextTrackCallback(callback) && mq.GetShotBeforeCurrentItem()) {
        Y_ENSURE(haveScState);
        mq.SetShotPlayed(mq.MutableCurrentItem(), /* played = */ true, /* onlyFirstAvailable = */ true);
        mCtx.SetOffsetMs(0);
        LOG_INFO(logger) << "AddMusicContentRequest: next after shot callback";
    } else if (IsNextTrackCallback(callback) && !applyArgs.HasRadioRequest() && !mq.IsGenerative()) {
        Y_ENSURE(haveScState);

        if (mq.IsRadio()) {
            const size_t freshQueueSize = applyRequest.LoadValueFromExpPrefix(
                EXP_HW_MUSIC_THIN_CLIENT_RADIO_FRESH_QUEUE_SIZE_PREFIX, /* defaultValue = */ RADIO_BATCH_SIZE);
            // both freshQueueSize and QueueSize() take into account currently playing track so we're adding 1 to RADIO_BATCH_SIZE
            if (freshQueueSize + mq.QueueSize() <= RADIO_BATCH_SIZE + 1) {
                mq.ClearQueue();
            }
        }

        auto result = mq.ChangeToNextTrack();
        result = TryFixPrefetchBugIfNeeded(logger, applyRequest, mq, result);
        LOG_INFO(logger) << "AddMusicContentRequest: next callback result " << result;

        mCtx.SetOffsetMs(0);
    } else if (IsRecoveryCallback(callback)) {
        // TODO(vitvlkv): Write a good convertion function StructToProto
        const auto payloadStruct = callback->GetPayload();
        const auto payloadJson = JsonFromProto(payloadStruct);
        TRecoveryCallbackPayload payload;
        JsonToProto(payloadJson, payload, /* validateUtf8= */ true, /* ignoreUnknownFields= */ false);

        mq.InitPlaybackFromContext(payload.GetPlaybackContext());
        auto musicConfig = CreateMusicConfig(applyRequest.ExpFlags());
        mq.SetConfig(musicConfig);
        mq.SetFiltrationMode(applyRequest.FiltrationMode());
        scState.ClearReaskState(); // TODO(vitvlkv): Shall we do this here?

        if (payload.HasPaged()) {
            // Do nothing
        } else if (payload.HasRadio()) {
            const auto& batchId = payload.GetRadio().GetBatchId();
            mq.SetRadioBatchId(batchId);
            mq.SetOldRadioBatchId(batchId);

            const auto& sessionId = payload.GetRadio().GetSessionId();
            mq.SetRadioSessionId(sessionId);
            mq.SetOldRadioSessionId(sessionId);
        } else {
            ythrow yexception() << "Expected either Paged or Radio state";
        }

    } else if (playerCommand == TMusicArguments_EPlayerCommand_NextTrack) {
        if (const auto frameProto = applyRequest.Input().FindSemanticFrame(NAlice::NMusic::PLAYER_NEXT_TRACK)) {
            const auto frame = TFrame::FromProto(*frameProto);
            if (const auto slot = frame.FindSlot(NAlice::NMusic::SLOT_SET_PAUSE)) {
                if (const auto maybeValue = slot->Value.As<bool>()) {
                    mCtx.SetNeedSetPauseAtStart(*maybeValue);
                }
            }
        }
        Y_ENSURE(haveScState);
        if (mq.GetRepeatType() == RepeatTrack) {
            mq.RepeatPlayback(RepeatNone);
        }
        if (mq.IsRadio() && applyRequest.HasExpFlag(NExperiments::EXP_MUSIC_THIN_CLIENT_RADIO_SLOW_NEXT_TRACK)) {
            mq.ClearQueue();
        }

        auto result = (applyArgs.GetIsShotPlaying() || mq.IsGenerative()) ? ETrackChangeResult::SameTrack : mq.ChangeToNextTrack();
        result = TryFixPrefetchBugIfNeeded(logger, applyRequest, mq, result);
        LOG_INFO(logger) << "AddMusicContentRequest: next result " << result;

        if (applyArgs.GetIsShotPlaying()) {
            mq.SetShotPlayed(mq.MutableCurrentItem(), /* played = */ true, /* onlyFirstAvailable = */ true);
        }
        mCtx.SetOffsetMs(0);

        if (mq.IsGenerative()) {
            mCtx.SetNeedGenerativeSkip(true);
        } else if (applyArgs.GetIsShotPlaying()) {
            mCtx.SetNeedToSendShotSkipFeedback(true);
        } else if (mq.IsRadio()) {
            mq.ClearQueue();
            LOG_INFO(logger) << "Cleared all items in the MusicQueue (because of radio skip)";
            mCtx.SetNeedRadioSkip(true);
        }
    } else if (playerCommand == TMusicArguments_EPlayerCommand_PrevTrack) {
        if (const auto frameProto = applyRequest.Input().FindSemanticFrame(NAlice::NMusic::PLAYER_PREV_TRACK)) {
            const auto frame = TFrame::FromProto(*frameProto);
            if (const auto slot = frame.FindSlot(NAlice::NMusic::SLOT_SET_PAUSE)) {
                if (const auto maybeValue = slot->Value.As<bool>()) {
                    mCtx.SetNeedSetPauseAtStart(*maybeValue);
                }
            }
        }
        Y_ENSURE(haveScState);
        if (mq.GetRepeatType() == RepeatTrack) {
            mq.RepeatPlayback(RepeatNone);
        }
        auto result = mq.ChangeToPrevTrack();
        mCtx.SetOffsetMs(0);
        LOG_INFO(logger) << "AddMusicContentRequest: prev result " << result;
    } else if (playerCommand == TMusicArguments_EPlayerCommand_ChangeTrackNumber) {
        Y_ENSURE(haveScState);
        mq.ClearQueue();
        mCtx.SetOffsetMs(0);
        mq.SetNextPageIndex(0);
        mq.SetTrackOffsetIndex(applyArgs.GetPlaybackOptions().GetTrackOffsetIndex());
    } else if (playerCommand == TMusicArguments_EPlayerCommand_ChangeTrackVersion) {
        Y_ENSURE(haveScState);
        if (applyArgs.GetPlaybackOptions().GetTrackVersion()) {
            mCtx.MutableOnDemandRequest()->SetTrackId(mq.CurrentItem().GetTrackId());
            AddTrackFullInfoFlag(ctx.ServiceCtx);
            LOG_INFO(logger) << "Added need_track_full_info flag";
        } else {
            mCtx.MutableOnDemandRequest()->SetSearchText(mq.CurrentItem().GetTitle());
            AddTrackSearchFlag(ctx.ServiceCtx);
            LOG_INFO(logger) << "Added need_track_search flag";
        }
    } else if (playerCommand == TMusicArguments_EPlayerCommand_Continue) {
        Y_ENSURE(haveScState);
        UpdateCurrentTrackOffsetMs(applyRequest, mCtx);
        LOG_INFO(logger) << "AddMusicContentRequest: continue current track";
        if (mq.IsGenerative()) {
            mCtx.SetNeedGenerativeContinue(true);
        }
    } else if (playerCommand == TMusicArguments_EPlayerCommand_Like) {
        Y_ENSURE(haveScState);
        Y_ENSURE(applyArgs.GetOnboardingTracksGame());
        // Tracks game can only happen with radio playing
        Y_ENSURE(mq.IsRadio());
        auto result = mq.ChangeToNextTrack();
        mCtx.SetOffsetMs(0);
        LOG_INFO(logger) << "AddMusicContentRequest: like result " << result;

        mq.ClearQueue();
        LOG_INFO(logger) << "Cleared all items in the MusicQueue (because of radio like)";
        mCtx.SetNeedOnboardingRadioLikeDislike(true);
    } else if (playerCommand == TMusicArguments_EPlayerCommand_Dislike) {
        Y_ENSURE(haveScState);
        if (mq.GetRepeatType() == RepeatTrack) {
            mq.RepeatPlayback(RepeatNone);
        }

        ETrackChangeResult result;
        if (!IsAudioPlayerPlaying(applyRequest.BaseRequestProto().GetDeviceState()) ||
            applyArgs.GetIsShotPlaying()) {
            result = ETrackChangeResult::SameTrack;
        } else {
            result = mq.IsGenerative() ? ETrackChangeResult::SameTrack : mq.ChangeToNextTrack();
            result = TryFixPrefetchBugIfNeeded(logger, applyRequest, mq, result);
            mCtx.SetOffsetMs(0);
        }

        LOG_INFO(logger) << "AddMusicContentRequest: dislike result " << result;

        mq.SetIsEndOfContent(result == ETrackChangeResult::EndOfContent);

        if (mq.IsGenerative()) {
            mCtx.SetNeedGenerativeDislike(true);
        } else if (applyArgs.GetIsShotPlaying()) {
            mCtx.SetNeedShotDislike(true);
        } else if (mq.IsRadio()) {
            mq.ClearQueue();
            LOG_INFO(logger) << "Cleared all items in the MusicQueue (because of radio dislike)";
            if (applyArgs.GetOnboardingTracksGame()) {
                mCtx.SetNeedOnboardingRadioLikeDislike(true);
            } else {
                mCtx.SetNeedRadioDislike(true);
            }
        }
    } else if (playerCommand == TMusicArguments_EPlayerCommand_Shuffle) {
        Y_ENSURE(haveScState);
        mq.ShufflePlayback(ctx.Rng);
        LOG_INFO(logger) << "AddMusicContentRequest: shuffle current content, "
                         << TContentId::EContentType_Name(mq.ContentId().GetType()) << ":" << mq.ContentId().GetId();
    } else if (playerCommand == TMusicArguments_EPlayerCommand_Replay) {
        Y_ENSURE(haveScState);
        mCtx.SetOffsetMs(0);
        if (mq.CurrentItem().GetRememberPosition()) {
            LOG_INFO(logger) << "AddMusicContentRequest: replay whole entity because of saved progress logic";
            mq.ClearQueue();
            mq.SetNextPageIndex(0);
        } else {
            LOG_INFO(logger) << "AddMusicContentRequest: replay current track";
        }
    } else if (mCtx.GetAccountStatus().GetHasMusicSubscription()) {
        TMaybe<TContentId> contentId;
        NAlice::NData::NMusic::TContentInfo contentInfo;
        if (applyArgs.HasPlaylistRequest()) {
            Y_ENSURE(!applyArgs.HasRadioRequest());
            *mCtx.MutablePlaylistRequest() = applyArgs.GetPlaylistRequest();
            //Content id will be completed after search/special_playlist request
            TContentId playlistIncompleteId;
            playlistIncompleteId.SetType(TContentId_EContentType_Playlist);
            contentId = playlistIncompleteId;
            if (mCtx.GetPlaylistRequest().GetPlaylistType() == TPlaylistRequest_EPlaylistType_Normal) {
                AddPlaylistSearchFlag(ctx.ServiceCtx);
            } else if (mCtx.GetPlaylistRequest().GetPlaylistType() == TPlaylistRequest_EPlaylistType_Special) {
                AddSpecialPlaylistFlag(ctx.ServiceCtx);
            } else {
                ythrow yexception() << "Unexpected playlist type " <<
                                    TPlaylistRequest_EPlaylistType_Name(mCtx.GetPlaylistRequest().GetPlaylistType());
            }
            LOG_INFO(logger) << "Added need_playlist_search flag";
        } else if (applyArgs.HasGenerativeRequest()) {
            const auto& generativeRequest = applyArgs.GetGenerativeRequest();
            LOG_INFO(logger) << "Has GenerativeRequest: StationId=" << generativeRequest.GetStationId();
            mCtx.SetOffsetMs(0);
            *mCtx.MutableGenerativeRequest() = generativeRequest;

            TContentId generativeContentId;
            generativeContentId.SetType(TContentId_EContentType_Generative);
            generativeContentId.SetId(generativeRequest.GetStationId());
            contentId = generativeContentId;
        } else if (applyArgs.HasRadioRequest()) {
            const auto& radioRequest = applyArgs.GetRadioRequest();
            LOG_INFO(logger) << "Has RadioRequest: StationIds=" << JoinSeq(",", radioRequest.GetStationIds());
            *mCtx.MutableRadioRequest() = radioRequest;

            TContentId radioContentId;
            radioContentId.SetType(TContentId_EContentType_Radio);
            for (const auto& id : radioRequest.GetStationIds()) {
                TStringBuf filterType;
                TStringBuf filterValue;
                TStringBuf(id).TrySplit(':', filterType, filterValue);
                Y_ENSURE(!filterType.empty());
                Y_ENSURE(!filterValue.empty());
                if (filterType == "track" && IsUgcTrackId(filterValue)) {
                    radioContentId.AddIds("user:onyourwave");
                } else {
                    radioContentId.AddIds(id);
                }
            }
            contentId = radioContentId;

            AddFeedbackRadioStartedFlag(ctx.ServiceCtx);
            LOG_INFO(logger) << "Added need_feedback_radio_started flag";
        } else {
            const auto& musicSearchResult = applyArgs.GetMusicSearchResult();
            const auto& objType = musicSearchResult.GetContentType();
            const auto& objId = musicSearchResult.GetContentId();
            contentId = ContentIdFromText(objType, objId);
            if (const auto& name = musicSearchResult.GetName(); !name.empty()) {
                contentInfo.SetName(name);
            } else if (const auto& title = musicSearchResult.GetTitle(); !title.empty()) {
                contentInfo.SetTitle(title);
            }
            if (!contentId) {
                LOG_ERR(logger) << "Failed to construct content id from type = " << objType << ", id = " << objId;
                return;
            }

            AddNoveltyAlbumSearchFlagIfNeeded(logger, contentId.GetRef(), applyRequest, mCtx, ctx.ServiceCtx);
        }

        const auto& playbackOptions = applyArgs.GetPlaybackOptions();

        LOG_INFO(logger) << "AddMusicContentRequest: type = " << TContentId_EContentType_Name(contentId->GetType())
                         << ", id = " << contentId->GetId() << ", playbackOptions = " << playbackOptions
                         << ", filtrationMode = "
                         << NScenarios::TUserPreferences_EFiltrationMode_Name(applyRequest.FiltrationMode());

        mq.InitPlayback(
            *contentId,
            ctx.Rng,
            playbackOptions,
            applyRequest.HasExpFlag(EXP_HW_MUSIC_THIN_CLIENT_SHOTS_FOR_ALL),
            contentInfo
        );
        auto musicConfig = CreateMusicConfig(applyRequest.ExpFlags());
        mq.SetConfig(musicConfig);
        mq.SetFiltrationMode(applyRequest.FiltrationMode());
        mCtx.SetFirstPlay(true);
        mCtx.SetOffsetMs(applyArgs.GetOffsetMs());
        scState.ClearReaskState();

        // For radio we don't go to MUSIC_SCENARIO_THIN_FIND_TRACK_IDX_PROXY_PREPARE
        // because start_from_track_id is used in handler while starting radio session
        if (!mq.GetStartFromTrackId().empty() && contentId->GetType() != TContentId_EContentType_Radio &&
            contentId->GetType() != TContentId_EContentType_Track || ShouldUseResumeFrom(applyRequest, applyArgs, contentId))
        {
            TFindTrackIdxRequest findTrackIdxRequest;
            if (contentId->GetType() == TContentId_EContentType_Album) {
                findTrackIdxRequest.SetContentType(TFindTrackIdxRequest_EContentType_Album);
            } else if (contentId->GetType() == TContentId_EContentType_Artist) {
                findTrackIdxRequest.SetContentType(TFindTrackIdxRequest_EContentType_Artist);
            } else if (contentId->GetType() == TContentId_EContentType_Playlist) {
                findTrackIdxRequest.SetContentType(TFindTrackIdxRequest_EContentType_Playlist);
            } else {
                findTrackIdxRequest.SetContentType(TFindTrackIdxRequest_EContentType_Undefined);
            }
            if (!mq.GetStartFromTrackId().empty()) {
                findTrackIdxRequest.SetTrackId(mq.GetStartFromTrackId());
            } else {
                findTrackIdxRequest.SetShouldUseResumeFrom(true);
            }
            *mCtx.MutableFindTrackIdxRequest() = findTrackIdxRequest;
            AddFindTrackIdxFlag(ctx.ServiceCtx);
            LOG_INFO(logger) << "Added need_find_track_idx flag";
        }
    }
    if (!IsNextTrackCallback(callback) || !mq.GetShotBeforeCurrentItem()) {
        TRepeatedSkip{scState, logger}.HandlePlayerCommand(playerCommand);
    }

    mCtx.SetRequestSource(applyRequest.BaseRequestProto().GetRequestSource());

    ForwardRequestAndMeta(ctx.ServiceCtx, applyRequest.Proto());
    AddMusicContext(ctx.ServiceCtx, mCtx);
    AddMusicThinClientFlag(ctx.ServiceCtx);
}

void TContentProxyPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};
    const auto& applyArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
    TMusicContext mCtx = GetMusicContext(ctx.ServiceCtx);
    Y_ENSURE(mCtx.HasAccountStatus());
    auto& scState = *mCtx.MutableScenarioState();
    TryInitPlaybackContextBiometryOptions(logger, scState);
    TMusicQueueWrapper mq(logger, *scState.MutableQueue());

    if (request.IsNewSession()) {
        LOG_INFO(logger) << "Reset music onboarding state on new session";
        scState.ClearOnboardingState();
    }

    LOG_INFO(logger) << mq.ToStringDebug();
    if (!mCtx.GetAccountStatus().GetUid().Empty() && !mCtx.GetAccountStatus().GetHasMusicSubscription()) {
        // TODO(klim-roma): check plus in guest mode
        LOG_INFO(logger) << "Preparing billing request promo request...";

        THttpHeaders billingHeaders;
        if (const auto billingExpFlagsJson = NAlice::NBilling::ExpFlagsToBillingHeader(request.ExpFlags())) {
            LOG_INFO(logger) << "Billing experiment flags json: " << *billingExpFlagsJson;
            billingHeaders.AddHeader("X-Yandex-ExpFlags", Base64Encode(*billingExpFlagsJson));
        }
        auto billingRequestPromo = THttpProxyNoRtlogRequestBuilder(NAlice::NBilling::AppHostRequestPromoUrlPath(), ctx.RequestMeta,
                                                            ctx.Ctx.Logger(), "MUSIC_SCENARIO_BILLING_PROXY")
            .SetMethod(NAppHostHttp::THttpRequest::Post)
            .SetUseOAuth()
            .AddHeaders(billingHeaders)
            .Build();
        AddMusicProxyRequest(ctx, billingRequestPromo, MUSIC_BILLING_REQUEST_ITEM);

        AddMusicContext(ctx.ServiceCtx, mCtx);
        ForwardRequestAndMeta(ctx.ServiceCtx, requestProto);
        return;
    }

    if (HaveHttpResponse(ctx, MUSIC_TRACK_FULL_INFO_RESPONSE_ITEM)) {
        const auto response = GetRawHttpResponse(ctx, MUSIC_TRACK_FULL_INFO_RESPONSE_ITEM);
        ParseTrackFullInfoResponse(response, mq, applyArgs.GetPlaybackOptions().GetTrackVersion());
    }

    if (HaveHttpResponse(ctx, MUSIC_TRACK_SEARCH_RESPONSE_ITEM)) {
        const auto response = GetRawHttpResponse(ctx, MUSIC_TRACK_SEARCH_RESPONSE_ITEM);
        ParseTrackSearchResponse(response, mq);
    }

    if (HaveHttpResponse(ctx, MUSIC_PLAYLIST_SEARCH_RESPONSE_ITEM)) {
        const auto response = GetRawHttpResponse(ctx, MUSIC_PLAYLIST_SEARCH_RESPONSE_ITEM);
        TString playlistName;
        ParsePlaylistSearchResponse(response, mq, playlistName);
        if (!playlistName.empty()) {
            mCtx.MutablePlaylistRequest()->SetPlaylistName(playlistName);
        } else {
            mCtx.MutableContentStatus()->SetErrorVer2(ErrorNotFoundVer2);
            LOG_INFO(logger) << "ErrorNotFoundVer2 was set";
        }
    }

    if (HaveHttpResponse(ctx, MUSIC_SPECIAL_PLAYLIST_RESPONSE_ITEM)) {
        const auto response = GetRawHttpResponse(ctx, MUSIC_SPECIAL_PLAYLIST_RESPONSE_ITEM);
        TString playlistName;
        ParseSpecialPlaylistResponse(response, mq, playlistName);
        if (!playlistName.empty()) {
            mCtx.MutablePlaylistRequest()->SetPlaylistName(playlistName);
        } else {
            LOG_INFO(logger) << "Playlist response is pumpkin, need to fallback to radio.";
            TContentId id;
            id.SetType(TContentId_EContentType_Radio);
            id.AddIds(USER_RADIO_STATION_ID);
            mq.SetContentId(id);
        }
    }

    if (HaveHttpResponse(ctx, MUSIC_NOVELTY_ALBUM_SEARCH_RESPONSE_ITEM)) {
        const auto response = GetRawHttpResponse(ctx, MUSIC_NOVELTY_ALBUM_SEARCH_RESPONSE_ITEM);
        ParseNoveltyAlbumSearchResponse(response, mq);
    }

    // Can happen only with MusicPlayObjectHandler and StartFromTrackId field in MusicPlay TSF
    if (HaveHttpResponse(ctx, MUSIC_FIND_TRACK_IDX_RESPONSE_ITEM)) {
        const auto response = GetRawHttpResponse(ctx, MUSIC_FIND_TRACK_IDX_RESPONSE_ITEM);
        ParseFindTrackIdxResponse(response, mCtx.GetFindTrackIdxRequest(), mq);
    }

    if (ShouldReturnContentResponse(mCtx, mq)) {

        if (mq.GetTrackOffsetIndex() != 0) {
            const auto [firstPageSize, nextPageIdx] = FindOptimalPageParameters(mq, mq.GetTrackOffsetIndex());
            LOG_INFO(logger) << "Optimal page parameters: track offset index = " << mq.GetTrackOffsetIndex()
                             << ", first page size = " << firstPageSize
                             << ", current page index = " << nextPageIdx;
            mCtx.SetFirstRequestPageSize(firstPageSize);
            // Set next page idx for further incrementing it and using with config's page size
            mq.SetNextPageIndex(nextPageIdx);
        }

        TryUpdateContentIdIfNeedSimilar(logger, request, mq);

        const auto& pbCtx = scState.GetQueue().GetPlaybackContext();
        LOG_INFO(logger) << "Making content proxy request "
                         << TContentId::EContentType_Name(pbCtx.GetContentId().GetType()) << ':'
                         << pbCtx.GetContentId().GetId() << " needRadioSkip=" << mCtx.GetNeedRadioSkip();
        const auto biometryData = ProcessBiometryOrFallback(logger, request, TStringBuf{applyArgs.GetAccountStatus().GetUid()});
        const auto& requesterUserId = mq.GetBiometryUserIdOrFallback(applyArgs.GetAccountStatus().GetUid());
        auto metaProvider = MakeRequestMetaProviderFromPlaybackBiometry(ctx.RequestMeta, mq.GetBiometryOptions());
        auto httpReq = PrepareContentRequest(request, mq, mCtx, metaProvider,
                                             ctx.Ctx.Logger(), biometryData,    // TODO(klim-roma): probably use PlaybackMode from scState instead of biometryData
                                             requesterUserId, Nothing(), applyArgs.GetPlayerCommand());
        if (mq.ContentId().GetType() == TContentId_EContentType_Radio) {
            ctx.ServiceCtx.AddFlag("need_music_web_backend");
            AddMusicProxyRequest(ctx, httpReq, MUSIC_RADIO_REQUEST_ITEM);
        } else if (mq.ContentId().GetType() == TContentId_EContentType_Generative) {
            AddMusicProxyRequest(ctx, httpReq, MUSIC_GENERATIVE_REQUEST_ITEM);
        } else {
            ctx.ServiceCtx.AddFlag("need_music_web_ext_backend");
            if (request.HasExpFlag(NExperiments::EXP_HW_MUSIC_TRACK_CACHE) &&
                mq.ContentId().GetType() == TContentId_EContentType_Track &&
                mCtx.GetFirstRequestPageSize() == 0)
            {
                if (const ui64 regionId = mCtx.GetAccountStatus().GetMusicSubscriptionRegionId(); regionId != 0) {
                    const ui64 hash = mq.GetContentHash(mCtx.GetAccountStatus().GetMusicSubscriptionRegionId());

                    auto cachalotGetRequest = NAlice::NAppHostServices::TCachalotCache::MakeGetRequest(ToString(hash), CACHALOT_MUSIC_SCENARIO_STORAGE_TAG);
                    LOG_INFO(logger) << "Adding cachalot get request with hash " << hash;
                    ctx.ServiceCtx.AddProtobufItem(cachalotGetRequest, CONTENT_PROXY_CACHE_GET_REQUEST_ITEM);
                    ctx.ServiceCtx.AddBalancingHint(CACHE_LOOKUP_NODE_NAME, hash);
                } else {
                    LOG_ERR(logger) << "Got empty music subscription region id";
                }
            }
            switch (mq.ContentId().GetType()) {
                case TContentId_EContentType_Album:
                    AddMusicProxyRequest(ctx, httpReq, MUSIC_ALBUM_REQUEST_ITEM);
                    break;
                case TContentId_EContentType_Artist:
                    AddMusicProxyRequest(ctx, httpReq, MUSIC_ARTIST_REQUEST_ITEM);
                    break;
                default:
                    AddMusicProxyRequest(ctx, httpReq);
                    break;
            }
        }
    } else {
        LOG_INFO(logger) << "Skipping content proxy request";
    }

    const auto& playerCommand = applyArgs.GetPlayerCommand();
    const auto& biometryResult = applyArgs.GetBiometryResult();
    auto isClientBiometryModeApplyRequest = IsClientBiometryModeApplyRequest(logger, applyArgs);
    bool isShotPlaying = applyArgs.GetIsShotPlaying();
    const bool enableCrossDc = request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);
    if ((playerCommand == TMusicArguments_EPlayerCommand_Like || playerCommand == TMusicArguments_EPlayerCommand_Dislike) &&
        !biometryResult.GetIsGuestUser() && !isShotPlaying && !mq.IsGenerative())
    {
        auto musicRequestModeInfo = MakeMusicRequestModeInfoFromMusicArgs(applyArgs, scState, EAuthMethod::UserId, isClientBiometryModeApplyRequest);
        auto userId = musicRequestModeInfo.RequesterUserId;
        const bool isLike = playerCommand == TMusicArguments_EPlayerCommand_Like;
        auto likeDislikeReq = NMusic::MakeMusicLikeDislikeTrackFromQueueRequest(
            request, ctx.RequestMeta, request.ClientInfo(),
            ctx.Ctx.Logger(), userId, isLike, enableCrossDc, musicRequestModeInfo
        );
        // TODO(jan-fazli): rename MUSIC_DISLIKE_REQUEST_ITEM, cause it is now also used for likes; same with flag
        AddMusicProxyRequest(ctx, likeDislikeReq, MUSIC_DISLIKE_REQUEST_ITEM);
        AddNeedDislikeTrackFlag(ctx.ServiceCtx);
        LOG_INFO(logger) << "Made " << (isLike ? "like" : "dislike") << " track request";

        if (mCtx.GetNeedOnboardingRadioLikeDislike()) {
            Y_ENSURE(mq.IsRadio());
            auto radioFeedbackReq = NMusic::PrepareLikeDislikeRadioFeedbackProxyRequest(
                mq, ctx.RequestMeta, request.ClientInfo(),
                ctx.Ctx.Logger(), request, scState, isLike, enableCrossDc
            );
            AddMusicProxyRequest(ctx, radioFeedbackReq, MUSIC_RADIO_FEEDBACK_LIKE_DISLIKE_REQUEST_ITEM);
            LOG_INFO(logger) << "Made " << (isLike ? "like" : "dislike") << " radio feedback request";
        }
    }

    if (mCtx.GetNeedShotDislike()) {
        auto musicRequestModeInfo = MakeMusicRequestModeInfoFromMusicArgs(applyArgs, scState, EAuthMethod::UserId, isClientBiometryModeApplyRequest);
        auto dislikeReq = MakeShotsLikeDislikeFeedbackProxyRequest(
                              mq, ctx.RequestMeta, request.ClientInfo(), logger, mq.GetBiometryUserId(),
                              /* isLike = */ false, enableCrossDc, musicRequestModeInfo);
        AddMusicProxyRequest(ctx, dislikeReq.first, dislikeReq.second);
        LOG_INFO(logger) << "Made dislike shot feedback request";

        if (request.BaseRequestProto().GetDeviceState().GetAudioPlayer().GetPlayerState() == TDeviceState_TAudioPlayer_TPlayerState_Playing) {
            mq.SetShotPlayed(mq.MutableCurrentItem(), /* played = */ true, /* onlyFirstAvailable = */ true);
        }
    }

    if (mCtx.GetNeedGenerativeSkip()) {
        AddGenerativeFeedbackProxyRequest(ctx, request, applyArgs, scState, mq, logger, GENERATIVE_FEEDBACK_TYPE_TIMESTAMP_SKIP);
        LOG_INFO(logger) << "Made skip generative feedback request";
    }
    if (mCtx.GetNeedGenerativeDislike()) {
        AddGenerativeFeedbackProxyRequest(ctx, request, applyArgs, scState, mq, logger, GENERATIVE_FEEDBACK_TYPE_TIMESTAMP_DISLIKE);
        LOG_INFO(logger) << "Made dislike generative feedback request";
    }
    if (mCtx.GetNeedGenerativeContinue()) {
        AddGenerativeFeedbackProxyRequest(ctx, request, applyArgs, scState, mq, logger, GENERATIVE_FEEDBACK_TYPE_STREAM_PLAY);
        LOG_INFO(logger) << "Made streamPlay generative feedback request";
    }

    if (request.Interfaces().GetSupportsShowView() && request.HasExpFlag(EXP_HW_MUSIC_SHOW_VIEW)) {
        const auto addRequest = [&ctx, &request]<typename THelper>{
            THelper helper{ctx, request};
            if (request.HasExpFlag(EXP_HW_MUSIC_CACHE_LIKES_TRACKS_REQUEST)) {
                helper.SetUseCache();
            }
            helper.AddRequest();
        };
        addRequest.operator()<TLikesTracksRequestHelper<ERequestPhase::Before>>();
        addRequest.operator()<TDislikesTracksRequestHelper<ERequestPhase::Before>>();
    }

    AddMusicContext(ctx.ServiceCtx, mCtx);
    ForwardRequestAndMeta(ctx.ServiceCtx, requestProto);
}

} // namespace NAlice::NHollywood::NMusic
