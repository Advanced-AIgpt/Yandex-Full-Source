#include "result_renders.h"

#include "consts.h"
#include "get_track_url_handles.h"
#include "multiroom.h"
#include "music_common.h"
#include "repeated_skip.h"
#include "unauthorized_user_directives.h"
#include "vsid.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/audio_play_builder/audio_play_builder.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/audio_play_builder/callback_payload_builder.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/content_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/download_info.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/download_info_parser.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/track_url_builder.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/xml_resp_parser.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/multiroom.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/saved_progress/saved_progress_helper.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/track_announce.h>

#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/analytics_info/analytics_info.h>
#include <alice/hollywood/library/scenarios/music/biometry/process_biometry.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/bedtime_tales.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/child_age_settings.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/intents.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/semantic_frames.h>
#include <alice/hollywood/library/scenarios/music/proto/callback_payload.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/show_view_builder/show_view_builder.h>
#include <alice/hollywood/library/scenarios/music/s3_animations/s3_animations.h>
#include <alice/hollywood/library/scenarios/music/util/onboarding.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/biometry/biometry_delegate.h>
#include <alice/hollywood/library/capability_wrapper/capability_wrapper.h>
#include <alice/hollywood/library/multiroom/multiroom.h>
#include <alice/hollywood/library/personal_data/personal_data.h>
#include <alice/hollywood/library/s3_animations/s3_animations.h>
#include <alice/hollywood/library/sound/sound_change.h>

#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/protos/endpoint/capability.pb.h>

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/library/analytics/common/utils.h>
#include <alice/library/billing/defs.h>
#include <alice/library/biometry/biometry.h>
#include <alice/library/data_sync/data_sync.h>
#include <alice/library/equalizer/equalizer.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/music/defs.h>
#include <alice/library/proto/protobuf.h>
#include <alice/library/scled_animations/scled_animations_directive_hw.h>
#include <alice/library/util/rng.h>

#include <library/cpp/digest/md5/md5.h>

#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NMusic {

namespace {

NJson::TJsonValue MakeJsonArtists(const NProtoBuf::RepeatedPtrField<TArtistInfo>& artists) {
    NJson::TJsonValue result{NJson::JSON_ARRAY};
    for (const auto& artist : artists) {
        auto& artistJson = result.AppendValue(NJson::JSON_MAP);
        artistJson["name"] = artist.GetName();
        artistJson["composer"] = artist.GetComposer();
    }
    return result;
}

NJson::TJsonValue MakeJsonLyricsInfo(const TQueueItem::TTrackInfo::TLyricsInfo& lyricsInfo) {
    NJson::TJsonValue result{NJson::JSON_MAP};
    result["has_available_sync_lyrics"] = lyricsInfo.GetHasAvailableSyncLyrics();
    result["has_available_text_lyrics"] = lyricsInfo.GetHasAvailableTextLyrics();
    return result;
}

} // namespace

NJson::TJsonValue MakeMusicAnswer(const TQueueItem& currentItem, const TContentId& contentId) {
    NJson::TJsonValue answerValue{NJson::JSON_MAP};
    answerValue["album_title"] = currentItem.GetTrackInfo().GetAlbumTitle();
    answerValue["album_year"] = currentItem.GetTrackInfo().GetAlbumYear();
    answerValue["artists"] = MakeJsonArtists(currentItem.GetTrackInfo().GetArtists());
    answerValue["genre"] = currentItem.GetTrackInfo().GetGenre();
    answerValue["subtype"] = currentItem.GetType();
    answerValue["track_title"] = currentItem.GetTitle();
    answerValue["type"] = ContentTypeToNLGType(contentId.GetType());
    answerValue["lyrics_info"] = MakeJsonLyricsInfo(currentItem.GetTrackInfo().GetLyricsInfo());
    switch (contentId.GetType()) {
        case TContentId_EContentType_Track:
            answerValue["title"] = currentItem.GetTitle();
            break;
        case TContentId_EContentType_Album:
            answerValue["title"] = currentItem.GetTrackInfo().GetAlbumTitle();
            break;
        case TContentId_EContentType_Artist:
            answerValue["name"] = TMusicQueueWrapper::ArtistName(currentItem);
            break;
        case TContentId_EContentType_Playlist:
            break;
        case TContentId_EContentType_Radio: {
            auto& filters = answerValue.InsertValue("filters", NJson::JSON_MAP);
            Y_ENSURE(!contentId.GetIds().empty());
            for (const auto& id : contentId.GetIds()) {
                TStringBuf filterType; // e.g. "mood"
                TStringBuf filterValue; // e.g. "sad"
                TStringBuf(id).TrySplit(":", filterType, filterValue);
                Y_ENSURE(!filterType.empty());
                Y_ENSURE(!filterValue.empty());
                filters[filterType] = NJson::TJsonArray();
                filters[filterType].AppendValue(filterValue);
            }
            break;
        }
        case TContentId_EContentType_Generative:
            answerValue["title"] = currentItem.GetTitle();
            answerValue["station"] = contentId.GetId();
            break;
        case TContentId_EContentType_FmRadio:
            answerValue["title"] = currentItem.GetTitle();
            answerValue["active"] = currentItem.GetFmRadioInfo().GetActive();
            answerValue["available"] = currentItem.GetFmRadioInfo().GetAvailable();
        case TContentId_EContentType_TContentId_EContentType_INT_MIN_SENTINEL_DO_NOT_USE_:
        case TContentId_EContentType_TContentId_EContentType_INT_MAX_SENTINEL_DO_NOT_USE_:
            break;
    }
    return answerValue;
}

namespace NImpl {

namespace {

constexpr TStringBuf RENDER_SUBSCRIPTION_REQUIRED = "render_thin_client_subscription_required";
constexpr TStringBuf RENDER_GENERIC_ERROR = "error";
constexpr TStringBuf RENDER_MUSIC_ERROR = "render_error__musicerror";
constexpr TStringBuf RENDER_UNAUTHORIZED = "render_error__unauthorized";
constexpr TStringBuf SET_GLAGOL_METADATA_DIRECTIVE_NAME = "set_glagol_metadata";
const TString PLAYER_ERROR_GIF_URI = "https://static-alice.s3.yandex.net/led-production/player/error.gif";
const TString PROMO_TITLE = "Яндекс.Плюс";
const TString PROMO_BODY = "Нажмите для активации " + PROMO_TITLE;

struct TExtraContentInfo {
    TMaybe<EContentAttentionVer2> ContentAttention;
    const TStringBuf ArtistName;
    const TStringBuf PlaylistTitle;
    bool IsPersonal;
    bool IsMultiroomRoomsNotSupportedAttention = false;
    bool IsMultiroomNotSupportedAttention = false;
    bool IsRestrictedContentSettingsAttention = false;
    bool IsRupStream = false;
    bool IsUnverifiedPlaylist = false;
    bool UsedSavedProgress = false;
    bool ShouldAddChildAgePromo = false;
    bool ShouldAddBedtimeTalesOnboarding = false;
    bool OmitObjectNameInNlgForAra = false;
    bool IsNeedSimilar = false;
};

void AddPlayerStopDirectives(TApplyResponseBuilder& builder) {
    auto& bodyBuilder = builder.GetResponseBodyBuilder() ? *builder.GetResponseBodyBuilder()
                                            : builder.CreateResponseBodyBuilder();

    bodyBuilder.AddClientActionDirective(TString("clear_queue"), NJson::TJsonValue());
    bodyBuilder.AddClientActionDirective(TString("go_home"), NJson::TJsonValue());
    bodyBuilder.AddClientActionDirective(TString("screen_off"), NJson::TJsonValue());
}

void AddCallbacksForPaged(TScenarioHandleContext& ctx, const TMusicContext& mCtx,const TMusicQueueWrapper& mq,
                          const TStringBuf uid, const bool incognito, TAudioPlayBuilder& audioPlayBuilder) {
    auto& logger = ctx.Ctx.Logger();
    const auto& currentItem = mq.CurrentItem();

    TCallbackPayloadBuilder onStartedBuilder;

    onStartedBuilder.AddPlayAudioEvent(
        mq.MakeFrom(), mq.ContentId(), currentItem.GetTrackId(), currentItem.GetTrackInfo().GetAlbumId(),
        currentItem.GetTrackInfo().GetAlbumType(), currentItem.GetPlayId(), uid, incognito, currentItem.GetRememberPosition());

    if (mCtx.GetNeedToSendShotSkipFeedback()) {
        auto shot = mq.GetLastShotOfCurrentItem();
        Y_ENSURE(shot, "Shot skip feedback was requested, but shot wasn't found");
        LOG_INFO(logger) << "Adding skip feedback to onStarted builder";
        onStartedBuilder.AddShotFeedbackEvent(*shot, TShotFeedbackEvent_EType_Skip, TString(uid));
    }

    auto onStartedStruct = onStartedBuilder.Build();

    auto playAudioStruct = TCallbackPayloadBuilder()
        .AddPlayAudioEvent(
            mq.MakeFrom(), mq.ContentId(), currentItem.GetTrackId(), currentItem.GetTrackInfo().GetAlbumId(),
            currentItem.GetTrackInfo().GetAlbumType(), currentItem.GetPlayId(), uid, incognito, currentItem.GetRememberPosition())
        .Build();

    audioPlayBuilder.AddOnStartedCallback(onStartedStruct)
        .AddOnStoppedCallback(playAudioStruct)
        .AddOnFinishedCallback(playAudioStruct)
        .AddOnFailedCallback(playAudioStruct);
}

void AddCallbacksForGenerative(const TMusicContext& mCtx, const TMusicQueueWrapper& mq,
                               const TMaybe<TString>& guestOAuthTokenEncrypted, TAudioPlayBuilder& audioPlayBuilder) {
    const auto& currentItem = mq.CurrentItem();
    const auto& trackId = currentItem.GetTrackId();
    const auto& generativeStationId = currentItem.GetGenerativeInfo().GetGenerativeStationId();
    auto from = mq.MakeFrom();

    auto onStartedPayloadBuilder = TCallbackPayloadBuilder();
    if (mCtx.GetOffsetMs() > 0) {
        onStartedPayloadBuilder.AddGenerativeFeedbackEvent(TGenerativeFeedbackEvent_EType_StreamPlay, generativeStationId,
                                                           trackId, guestOAuthTokenEncrypted);
    }
    audioPlayBuilder.AddOnStartedCallback(onStartedPayloadBuilder.Build());

    auto onStoppedPayloadBuilder = TCallbackPayloadBuilder()
        .AddGenerativeFeedbackEvent(TGenerativeFeedbackEvent_EType_StreamPause, generativeStationId, trackId, guestOAuthTokenEncrypted);
    audioPlayBuilder.AddOnStoppedCallback(onStoppedPayloadBuilder.Build());

    audioPlayBuilder.AddOnFailedCallback({})
        .AddOnFinishedCallback({});
}

void AddCallbacksForRadio(const TMusicContext& mCtx, const TMusicQueueWrapper& mq, const TStringBuf uid,
                          const TMaybe<TString>& guestOAuthTokenEncrypted, const bool incognito,
                          TAudioPlayBuilder& audioPlayBuilder) {
    const auto& currentItem = mq.CurrentItem();
    const auto& trackId = currentItem.GetTrackId();
    const auto& albumId = currentItem.GetTrackInfo().GetAlbumId();
    const auto& albumType = currentItem.GetTrackInfo().GetAlbumType();
    const auto& playId = currentItem.GetPlayId();

    auto from = mq.MakeFrom();

    const TString& radioSessionId = mq.GetRadioSessionId();

    auto onStartedPayloadBuilder = TCallbackPayloadBuilder()
        .AddPlayAudioEvent(
            from, mq.ContentId(), trackId, albumId, albumType, playId, uid,
            incognito, currentItem.GetRememberPosition(), radioSessionId,
            mq.GetRadioBatchId());

    if (mCtx.GetFirstPlay()) {
        onStartedPayloadBuilder.AddRadioStartedFeedbackEvent(mq.ContentId().GetId(), mq.GetRadioSessionId(), guestOAuthTokenEncrypted);
    }

    if (mCtx.GetNeedRadioSkip()) {
        const auto& prevItem = mq.PreviousItem();
        onStartedPayloadBuilder.AddRadioFeedbackEvent(TRadioFeedbackEvent_EType_Skip, mq.ContentId().GetId(),
                                                      mq.GetRadioBatchId(), prevItem.GetTrackId(),
                                                      prevItem.GetTrackInfo().GetAlbumId(), radioSessionId,
                                                      guestOAuthTokenEncrypted);
    }
    if (mCtx.GetNeedRadioDislike()) {
        const auto& prevItem = mq.PreviousItem();
        onStartedPayloadBuilder.AddRadioFeedbackEvent(TRadioFeedbackEvent_EType_Dislike, mq.ContentId().GetId(),
                                                      mq.GetRadioBatchId(), prevItem.GetTrackId(),
                                                      prevItem.GetTrackInfo().GetAlbumId(), radioSessionId,
                                                      guestOAuthTokenEncrypted);
    }

    onStartedPayloadBuilder.AddRadioFeedbackEvent(TRadioFeedbackEvent_EType_TrackStarted, mq.ContentId().GetId(),
                                                  mq.GetRadioBatchId(), trackId, albumId, radioSessionId,
                                                  guestOAuthTokenEncrypted);

    audioPlayBuilder.AddOnStartedCallback(onStartedPayloadBuilder.Build());

    auto onFinishedPayload = TCallbackPayloadBuilder()
        .AddPlayAudioEvent(
            from, mq.ContentId(), trackId, albumId, albumType, playId, uid,
            incognito, currentItem.GetRememberPosition(), radioSessionId, mq.GetRadioBatchId())
        .AddRadioFeedbackEvent(TRadioFeedbackEvent_EType_TrackFinished, mq.ContentId().GetId(),
                               mq.GetRadioBatchId(), trackId, albumId, radioSessionId,
                               guestOAuthTokenEncrypted)
        .Build();
    audioPlayBuilder.AddOnFinishedCallback(onFinishedPayload);

    auto playAudioStruct = TCallbackPayloadBuilder()
        .AddPlayAudioEvent(
            from, mq.ContentId(), trackId, albumId, albumType, playId, uid,
            incognito, currentItem.GetRememberPosition(), radioSessionId, mq.GetRadioBatchId())
        .Build();
    audioPlayBuilder.AddOnStoppedCallback(playAudioStruct)
        .AddOnFailedCallback(playAudioStruct);
}

void AddCallbacksForFmRadio(const TMusicQueueWrapper& mq, const TStringBuf uid,
                            const bool incognito, TAudioPlayBuilder& audioPlayBuilder) {
    const auto& currentItem = mq.CurrentItem();
    const auto& trackId = currentItem.GetTrackId();
    const auto& playId = currentItem.GetPlayId();
    auto from = mq.MakeFrom();

    auto onStartedPayloadBuilder = TCallbackPayloadBuilder()
        .AddPlayAudioEvent(
            from, mq.ContentId(), trackId, /* albumId = */ Nothing(), /* albumType = */ Nothing(),
            playId, uid, incognito, currentItem.GetRememberPosition());
    audioPlayBuilder.AddOnStartedCallback(onStartedPayloadBuilder.Build());

    auto onStoppedPayloadBuilder = TCallbackPayloadBuilder()
        .AddPlayAudioEvent(
            from, mq.ContentId(), trackId, /* albumId = */ Nothing(), /* albumType = */ Nothing(),
            playId, uid, incognito, currentItem.GetRememberPosition());
    audioPlayBuilder.AddOnStoppedCallback(onStoppedPayloadBuilder.Build());

    auto playAudioStruct = TCallbackPayloadBuilder()
        .AddPlayAudioEvent(
            from, mq.ContentId(), trackId, /* albumId = */ Nothing(), /* albumType = */ Nothing(),
            playId, uid, incognito, currentItem.GetRememberPosition())
        .Build();
    audioPlayBuilder.AddOnFailedCallback(playAudioStruct)
        .AddOnFinishedCallback({});
}

void TryMakeMultiroomDirectives(const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                TExtraContentInfo& extraInfo,
                                TApplyResponseBuilder& builder,
                                TMultiroomTokenWrapper& multiroomToken) {
    struct TCallbacks : TMultiroomCallbacks {
        TApplyResponseBuilder& Builder;
        TExtraContentInfo& ExtraInfo;
        TMultiroomTokenWrapper& MultiroomToken;

        TCallbacks(TApplyResponseBuilder& builder, TExtraContentInfo& extraInfo, TMultiroomTokenWrapper& multiroomToken)
            : Builder{builder}, ExtraInfo{extraInfo}, MultiroomToken{multiroomToken}
        {}

        void OnNeedDropMultiroomToken() override {
            MultiroomToken.DropToken();
        }

        void OnNeedStartMultiroom(NScenarios::TLocationInfo locationInfo) override {
            MultiroomToken.GenerateNewToken();

            NScenarios::TDirective directive;
            auto& startMultiroomDirective = *directive.MutableStartMultiroomDirective();
            *startMultiroomDirective.MutableLocationInfo() = std::move(locationInfo);
            *startMultiroomDirective.MutableMultiroomToken() = TString{MultiroomToken.GetToken()};
            Builder.GetOrCreateResponseBodyBuilder().AddDirective(std::move(directive));
        }

        void OnNeedStopMultiroom(TStringBuf sessionId) override {
            NScenarios::TDirective directive;
            directive.MutableStopMultiroomDirective()->SetMultiroomSessionId(sessionId.data(), sessionId.size());
            Builder.GetOrCreateResponseBodyBuilder().AddDirective(std::move(directive));
        }

        void OnMultiroomNotSupported() override {
            ExtraInfo.IsMultiroomNotSupportedAttention = true;
        }

        void OnMultiroomRoomsNotSupported() override {
            ExtraInfo.IsMultiroomRoomsNotSupportedAttention = true;
        }
    } callbacks{builder, extraInfo, multiroomToken};

    ProcessMultiroom(applyRequest, callbacks);
}

void MakeAudioPlayDirectiveAndNextTrackCallback(TScenarioHandleContext& ctx,
                                                const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                                TMusicContext& mCtx, TMusicQueueWrapper& mq,
                                                TApplyResponseBuilder& builder,
                                                const TStringBuf uid,
                                                const TShowViewBuilderSources sources,
                                                const TMusicShotsFastData& shots, IRng& rng,
                                                const TMultiroomTokenWrapper* multiroomToken = nullptr) {
    auto& logger = ctx.Ctx.Logger();
    auto& scState = *mCtx.MutableScenarioState();
    TryInitPlaybackContextBiometryOptions(logger, scState);
    TryAnnounceTrack(logger, applyRequest, builder, mq, mCtx, shots, rng);
    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();

    LOG_INFO(logger) << "Making audio_play directive and next track callback...";

    auto& currentItem = mq.MutableCurrentItem();
    auto& bodyBuilder = builder.GetResponseBodyBuilder() ? *builder.GetResponseBodyBuilder()
                                                         : builder.CreateResponseBodyBuilder();

    TMaybe<TString> prevTrackId = Nothing();
    TMaybe<TString> nextTrackId = Nothing();
    if (mq.HasPreviousItem()) {
        prevTrackId = mq.PreviousItem().GetTrackId();
    }
    if (mq.HasNextItem()) {
        nextTrackId = mq.NextItem().GetTrackId();
    }

    if (applyRequest.Interfaces().GetSupportsShowView() && applyRequest.HasExpFlag(EXP_HW_MUSIC_SHOW_VIEW)) {
        TShowViewBuilder showViewBuilder(logger, mq, sources, &applyRequest);
        bodyBuilder.AddShowViewDirective(showViewBuilder.BuildRenderData(), NScenarios::TShowViewDirective_EInactivityTimeout_Infinity, Content);

        // we need tts_placeholder directive after show_view directive: https://st.yandex-team.ru/HOLLYWOOD-1019
        bodyBuilder.AddTtsPlayPlaceholderDirective();
    }

    if (currentItem.HasTrackInfo() && !currentItem.GetTrackInfo().GetGenre().Empty()) {
        const TString seed = TString::Join("genre:", currentItem.GetTrackInfo().GetGenre());

        // can't play in multiroom session
        if (!WillPlayInMultiroomSession(applyRequest)) {
            auto directive = TryBuildEqualizerBandsDirective(
                applyArgs.GetEnvironmentState(), seed,
                applyRequest.ClientInfo().DeviceId,
                TEqualizerCapability_EPresetMode_MediaCorrection);
            if (directive) {
                bodyBuilder.AddDirective(std::move(*directive));
            }
        }
    }

    TMultiroomTokenWrapper defaultMultiroomToken{scState};
    const TMultiroomTokenWrapper* actualMultiroomToken = multiroomToken ? multiroomToken : &defaultMultiroomToken;
    auto audioPlayBuilder = TAudioPlayBuilder(currentItem, mCtx.GetOffsetMs(), mq.ContentId(),
                                              prevTrackId, nextTrackId, mq.GetMetadataShuffled(),
                                              mq.GetMetadataRepeatType(), mCtx.GetNeedSetPauseAtStart(),
                                              actualMultiroomToken);

    TMaybe<TString> guestOAuthTokenEncrypted = Nothing();
    if (mq.IsGuestBiometryPlaybackMode()) {
        Y_ENSURE(!mq.GetGuestOAuthTokenEncrypted().Empty());
        guestOAuthTokenEncrypted = mq.GetGuestOAuthTokenEncrypted();
    }

    if (mq.IsPaged()) {
        AddCallbacksForPaged(ctx, mCtx, mq, uid, mq.IsIncognitoBiometryPlaybackMode(), audioPlayBuilder);
    } else if (mq.IsGenerative()) {
        AddCallbacksForGenerative(mCtx, mq, guestOAuthTokenEncrypted, audioPlayBuilder);
    } else if (mq.IsRadio()) {
        AddCallbacksForRadio(mCtx, mq, uid, guestOAuthTokenEncrypted, mq.IsIncognitoBiometryPlaybackMode(), audioPlayBuilder);
    } else if (mq.IsFmRadio()) {
        AddCallbacksForFmRadio(mq, uid, mq.IsIncognitoBiometryPlaybackMode(), audioPlayBuilder);
    } else {
        ythrow yexception() << "Unsupported MusicQueue current state!";
    }
    bodyBuilder.AddDirective(audioPlayBuilder.BuildProto());

    // ATTN: We must put (if needed) NewSession BEFORE ResetAdd, because order matters here
    if (mCtx.GetFirstPlay() && mCtx.GetRequestSource() == NScenarios::TScenarioBaseRequest_ERequestSourceType_Default) {
        bodyBuilder.AddNewSessionStackAction();
        LOG_INFO(logger) << "Added NewSession to StackEngine actions";
    } else {
        LOG_INFO(logger) << "NOT Added NewSession to StackEngine actions, because FirstPlay="
                         << mCtx.GetFirstPlay() << ", RequestSource="
                         << NScenarios::TScenarioBaseRequest_ERequestSourceType_Name(mCtx.GetRequestSource());
    }

    auto resetAddBuilder = bodyBuilder.ResetAddBuilder();
    if (!mq.IsCurrentTrackLast() || mq.GetRepeatType() != NMusic::RepeatNone || !mq.IsAutoflowDisabled()) {
        resetAddBuilder.AddCallback(TString{MUSIC_THIN_CLIENT_NEXT_CALLBACK});
    }
    resetAddBuilder.AddRecoveryActionCallback(TString{MUSIC_THIN_CLIENT_RECOVERY_CALLBACK},
                                              mq.MakeRecoveryActionCallbackPayload());
}

void MakeAudioPlayDirectiveAndNextTrackCallbackWithFirstTrackAnalytics(
    TScenarioHandleContext& ctx, TMusicContext& mCtx, TMusicQueueWrapper& mq, TApplyResponseBuilder& builder, const TStringBuf uid,
    TMusicArguments::EPlayerCommand playerCommand,
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
    const TShowViewBuilderSources sources,
    const TMusicShotsFastData& shots, IRng& rng)
{
    MakeAudioPlayDirectiveAndNextTrackCallback(ctx, applyRequest, mCtx, mq, builder, uid, sources, shots, rng);
    CreateAndFillAnalyticsInfoForPlayerCommandWithFirstTrackObject(playerCommand, mq.CurrentItem(),
        *builder.GetResponseBodyBuilder(),
        applyRequest.ServerTimeMs(),
        mCtx.GetScenarioState().GetProductScenarioName(),
        mCtx.GetBatchOfTracksRequested(),
        mCtx.GetCacheHit());
}


void MakeShotDirectiveAndNextTrackCallback(TScenarioHandleContext& ctx, const TExtraPlayable_TShot& shot,
                                           const TStringBuf uid, TApplyResponseBuilder& builder,
                                           const TMusicContext& mCtx, const TMusicQueueWrapper& mq) {
    auto& logger = ctx.Ctx.Logger();
    auto& bodyBuilder = builder.GetResponseBodyBuilder() ? *builder.GetResponseBodyBuilder()
                                                         : builder.CreateResponseBodyBuilder();
    LOG_DEBUG(logger) << "Adding shotId = " << shot.GetId();
    auto resetAddBuilder = bodyBuilder.ResetAddBuilder();
    if (!shot.GetTtsText().empty()) {
        if (!bodyBuilder.HasListenDirective()) {
            bodyBuilder.AddRawTextWithButtonsAndVoice(shot.GetTtsText(), {});
        }
    } else {
        TQueueItem shotItem;
        shotItem.MutableUrlInfo()->SetUrl(shot.GetMdsUrl());
        shotItem.MutableUrlInfo()->SetUrlFormat(TTrackUrl::UrlFormatMp3);
        shotItem.SetTrackId(shot.GetId());
        shotItem.SetType("shot");
        shotItem.SetTitle(shot.GetTitle());
        shotItem.SetCoverUrl(shot.GetCoverUri());

        auto audioPlayBuilder = TAudioPlayBuilder(shotItem, mCtx.GetOffsetMs(), /* contentId = */ Nothing(),
                                                  /* prevTrackId = */ Nothing(), /* nextTrackId */ Nothing(),
                                                  mq.GetMetadataShuffled(), /* repeatMode */ Nothing());
        auto shotPlayFeedbackStruct = TCallbackPayloadBuilder()
            .AddShotFeedbackEvent(shot, TShotFeedbackEvent_EType_Play, TString(uid))
            .Build();
        auto shotDummyFeedbackStruct = TCallbackPayloadBuilder()
            .AddShotFeedbackEvent(shot, TShotFeedbackEvent_EType_Unknown, TString(uid))
            .Build();
        audioPlayBuilder
            .AddOnStartedCallback(shotDummyFeedbackStruct)
            .AddOnFailedCallback(shotDummyFeedbackStruct)
            .AddOnStoppedCallback(shotDummyFeedbackStruct)
            .AddOnFinishedCallback(shotPlayFeedbackStruct);

        bodyBuilder.AddDirective(audioPlayBuilder.BuildProto());
    }
    resetAddBuilder.AddCallback(TString{MUSIC_THIN_CLIENT_NEXT_CALLBACK});
    resetAddBuilder.AddRecoveryActionCallback(TString{MUSIC_THIN_CLIENT_RECOVERY_CALLBACK},
                                              mq.MakeRecoveryActionCallbackPayload());
}

void AddBassAttentionBlock(NJson::TJsonValue::TArray& bassBlocks,
                           const TStringBuf attention,
                           NJson::TJsonValue&& attentionData = NJson::EJsonValueType::JSON_NULL) {
    NJson::TJsonValue value;
    value["type"] = "attention";
    value["attention_type"] = attention;
    if (!attentionData.IsNull()) {
        value["data"] = std::move(attentionData);
    }
    bassBlocks.push_back(std::move(value));
}

TExtraContentInfo MakeExtraContentInfo(TScenarioHandleContext& ctx, const TMusicContext& mCtx,
                                       const NHollywood::TScenarioApplyRequestWrapper& applyRequest, IRng& rng) {
    auto& logger = ctx.Ctx.Logger();
    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();
    const auto& playbackContext = mCtx.GetScenarioState().GetQueue().GetPlaybackContext();
    const auto hasAttention = mCtx.GetContentStatus().GetAttentionVer2() != NMusic::NoAttention;
    TMaybe<EContentAttentionVer2> contentAttention = hasAttention ? mCtx.GetContentStatus().GetAttentionVer2()
                                                                  : TMaybe<EContentAttentionVer2>();
    const auto& musicSearchResult = applyArgs.GetMusicSearchResult();
    const auto& artistName = musicSearchResult.GetName();
    auto playlistTitle = mCtx.HasPlaylistRequest() ?
                         mCtx.GetPlaylistRequest().GetPlaylistName() :
                         applyArgs.GetMusicSearchResult().GetTitle();
    if (!playlistTitle && playbackContext.GetContentId().GetType() == TContentId_EContentType_Playlist) {
        playlistTitle = playbackContext.GetContentInfo().GetTitle();
    }

    bool isPersonal = musicSearchResult.GetIsPersonal();
    bool isRestrictedContentSettingsAttention = applyArgs.HasRadioRequest() && applyArgs.GetRadioRequest().GetNeedRestrictedContentSettingsAttention();

    bool isRupStream = false;
    bool isNeedSimilar = false;
    if (const auto frameProto = applyRequest.Input().FindSemanticFrame(MUSIC_PLAY_FRAME)) {
        isRupStream = FindIfPtr(frameProto->GetSlots(), [](const auto& slot) {
            return slot.GetName() == NAlice::NMusic::SLOT_STREAM;
        });
        isNeedSimilar = FindIfPtr(frameProto->GetSlots(), [](const auto& slot) {
            return slot.GetName() == NAlice::NMusic::SLOT_NEED_SIMILAR;
        });
    }

    bool omitObjectNameInNlgForAra =
        !applyRequest.HasExpFlag(NAlice::NExperiments::EXP_HW_MUSIC_KEEP_OBJECT_NAME_IN_NLG_FOR_ARA);

    return {contentAttention, artistName, playlistTitle, isPersonal,
            /* IsMultiroomRoomsNotSupportedAttention = */ false,
            /* IsMultiroomNotSupportedAttention = */ false,
            isRestrictedContentSettingsAttention, isRupStream, mCtx.GetUnverifiedPlaylist(), mCtx.GetUsedSavedProgress(),
            ShouldAddChildAgePromo(logger, applyRequest, rng), ShouldAddBedtimeTalesOnboarding(logger, applyRequest),
            omitObjectNameInNlgForAra, isNeedSimilar};
}

NJson::TJsonValue BuildFormAndSlots(
    const TMusicQueueWrapper& mq,
    const TExtraContentInfo& extraInfo,
    const bool disableExplicitContentAttention)
{
    NJson::TJsonValue stateJson;
    auto& form = stateJson.InsertValue("form", NJson::JSON_MAP);

    form["name"] = MUSIC_PLAY_FRAME;

    auto& slots = form.InsertValue("slots", NJson::JSON_ARRAY).GetArraySafe();
    auto& blocks = stateJson.InsertValue("blocks", NJson::JSON_ARRAY).GetArraySafe();

    if (mq.GetShuffle()) {
        AddBassAttentionBlock(blocks, "music_shuffle");
        NJson::TJsonValue shuffleSlot;
        shuffleSlot["name"] = "order";
        shuffleSlot["type"] = "order";
        shuffleSlot["source_text"] = "shuffle";
        shuffleSlot["value"] = "shuffle";
        shuffleSlot["optional"] = true;
        slots.push_back(std::move(shuffleSlot));
    }

    if (mq.GetRepeatType() != ERepeatType::RepeatNone) {
        AddBassAttentionBlock(blocks, "music_repeat");
        NJson::TJsonValue repeatSlot;
        repeatSlot["name"] = "repeat";
        repeatSlot["type"] = "repeat";
        repeatSlot["source_text"] = "repeat";
        repeatSlot["value"] = "repeat";
        repeatSlot["optional"] = true;
        slots.push_back(std::move(repeatSlot));
    }

    if (extraInfo.ContentAttention && !disableExplicitContentAttention) {
        TStringBuf attentionCode;
        switch (*extraInfo.ContentAttention) {
            case AttentionContainsAdultContentVer2:
                attentionCode = TStringBuf("contains-adult-content");
                break;
            case AttentionExplicitContentFilteredVer2:
                attentionCode = TStringBuf("explicit-content-filtered");
                break;
            case AttentionMayContainExplicitContentVer2:
                attentionCode = TStringBuf("may-contain-explicit-content");
                break;
            case AttentionForbiddenPodcast:
                // do nothing here
            case NoAttention:
            case EContentAttentionVer2_INT_MIN_SENTINEL_DO_NOT_USE_:
            case EContentAttentionVer2_INT_MAX_SENTINEL_DO_NOT_USE_:
                break;
        }
        NJson::TJsonValue attentionData;
        attentionData["code"] = attentionCode;
        AddBassAttentionBlock(blocks, TStringBuf("explicit_content"), std::move(attentionData));
    }

    if (!extraInfo.IsPersonal &&
        mq.ContentId().GetType() == TContentId_EContentType_Radio &&
        mq.ContentId().GetIds(0) == USER_RADIO_STATION_ID)
    {
        AddBassAttentionBlock(blocks, TStringBuf("is_general_playlist"));
    }

    if (extraInfo.IsPersonal) {
        // This will render proper response for request "включи мою музыку"
        NJson::TJsonValue personalitySlot;
        personalitySlot["name"] = "personality";
        personalitySlot["type"] = "personality";
        personalitySlot["value"] = "is_personal";
        personalitySlot["optional"] = true;
        slots.push_back(std::move(personalitySlot));
    }

    if (extraInfo.IsMultiroomRoomsNotSupportedAttention) {
        AddBassAttentionBlock(blocks, TStringBuf("multiroom_rooms_not_supported"));
    }

    if (extraInfo.IsMultiroomNotSupportedAttention) {
        AddBassAttentionBlock(blocks, TStringBuf("multiroom_not_supported"));
    }

    if (extraInfo.IsRestrictedContentSettingsAttention) {
        AddBassAttentionBlock(blocks, TStringBuf("restricted_content_settings"));
    }

    NJson::TJsonValue answer;
    answer["name"] = "answer";
    answer["type"] = "music_result";
    answer["optional"] = true;
    auto& answerValue = answer.InsertValue("value", MakeMusicAnswer(mq.CurrentItem(), mq.ContentId()));
    answerValue["omit_object_name"] = extraInfo.OmitObjectNameInNlgForAra;
    switch (mq.ContentId().GetType()) {
        case TContentId_EContentType_Artist:
            if (!extraInfo.ArtistName.empty()) {
                answerValue["name"] = extraInfo.ArtistName;
            }
            break;
        case TContentId_EContentType_Playlist:
            answerValue["title"] = extraInfo.PlaylistTitle;
            // I guess that "genre" is not needed here
            answerValue.EraseValue("genre");
            if (TPlaylistId::FromString(mq.ContentId().GetId())->Owner == PLAYLIST_ORIGIN_OWNER_UID) {
                NJson::TJsonValue specialPlaylistSlot;
                specialPlaylistSlot["name"] = "special_playlist";
                specialPlaylistSlot["type"] = "special_playlist";
                specialPlaylistSlot["value"] = "origin";
                slots.push_back(std::move(specialPlaylistSlot));
            }
            break;
        default:
            break;
    }
    slots.push_back(std::move(answer));

    return stateJson;
}

void RenderMusicNlgResult(TScenarioHandleContext& ctx, TMusicQueueWrapper& mq, const TExtraContentInfo& extraInfo,
                          const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                          TApplyResponseBuilder& builder, const TMusicFastData* fastData) {
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Rendering music_play render_result...";
    TBassResponseRenderer bassRenderer(applyRequest, applyRequest.Input(), builder, logger,
        /* suggestAutoAction= */ false);

    bassRenderer.SetContextValue("nlg_disabled", mq.IsNlgDisabled());

    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();
    const bool isFairyTaleRequest = applyArgs.GetFairyTaleArguments().GetIsFairyTaleSubscenario();
    auto stateJson = BuildFormAndSlots(mq, extraInfo, isFairyTaleRequest);
    if (fastData) {
        AddAudiobrandingAttention(stateJson, *fastData, applyRequest);
    }
    if (applyArgs.GetFairyTaleArguments().GetIsFairyTaleSubscenario()) {
        bassRenderer.SetContextValue("is_fairy_tale_subscenario", true);
    }
    bassRenderer.SetContextValue("is_ondemand_fairytale", applyArgs.GetFairyTaleArguments().GetIsOndemand());
    if (applyArgs.GetFairyTaleArguments().GetIsBedtimeTales()) {
        bassRenderer.SetContextValue("is_bedtime_tales", true);
    }
    if (applyArgs.HasAmbientSoundArguments()) {
        bassRenderer.SetContextValue("no_shuffle_repeat_in_nlg", true);
    }
    if (extraInfo.ShouldAddChildAgePromo) {
        AddChildAgePromoAttention(stateJson);
    }
    if (extraInfo.ShouldAddBedtimeTalesOnboarding) {
        AddBedtimeTalesOnboardingAttention(stateJson, true);
    }
    if (extraInfo.IsRupStream) {
        AddAttentionToJsonState(stateJson, "radio_filtered_stream");
    }
    if (applyRequest.Interfaces().GetHasMusicPlayer()) {
        AddAttentionToJsonState(stateJson, "supports_music_player");
    }
    if (extraInfo.IsUnverifiedPlaylist) {
        AddAttentionToJsonState(stateJson, TString{NAlice::NMusic::ATTENTION_UNVERIFIED_PLAYLIST});
    }
    if (extraInfo.UsedSavedProgress) {
        auto& bodyBuilder = builder.GetResponseBodyBuilder() ? *builder.GetResponseBodyBuilder()
                                                             : builder.CreateResponseBodyBuilder();
        TryAddCanStartFromTheBeginningAttention(logger, applyRequest, applyArgs, bodyBuilder, stateJson);
        AddAttentionToJsonState(stateJson, TString{NAlice::NMusic::ATTENTION_USED_SAVED_PROGRESS});
    }
    if (mq.ContentId().GetId() == ALBUM_PODUSHKI_SHOW_ID) {
        AddAttentionToJsonState(stateJson, "podushki_show");
    }
    if (extraInfo.IsNeedSimilar) {
        AddAttentionToJsonState(stateJson, "need_similar");
    }
    LOG_INFO(logger) << "FormAndSlots for NLG render_result is " << JsonToString(stateJson);

    const auto& fixlist = applyArgs.GetFixlist();
    const NJson::TJsonValue fixlistJson = JsonFromString(fixlist);
    if (fixlistJson.Has("nlg")) {
        bassRenderer.SetContextValue("fixlist", fixlistJson);
        bassRenderer.Render(TEMPLATE_MUSIC_PLAY, "fixlist", stateJson);
    } else {
        bassRenderer.Render(mq.IsFmRadio() ? TEMPLATE_FM_RADIO_PLAY : TEMPLATE_MUSIC_PLAY, "render_result", stateJson);
    }
}

void RenderNextTrackNlgResult(TScenarioHandleContext& ctx, const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                              TApplyResponseBuilder& builder, NMusic::TScenarioState& scState) {
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Rendering next track render_result NLG...";
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    TNlgData nlgData{logger, applyRequest};
    if (!TRepeatedSkip{scState, logger}.TryPropose(applyRequest, bodyBuilder, nlgData)) {
        return;
    }
    LOG_INFO(logger) << "Constructed nlg context=" << nlgData.Context;
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_PLAYER_NEXT_TRACK, "render_result", {}, nlgData);
}

void FillDownloadInfoResult(TScenarioHandleContext& ctx, const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                            const TMaybeRawHttpResponse& response, TMusicQueueWrapper& mq,
                            const TStringBuf uid, IRng& rng) {
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "FillDownloadInfoResult...";
    auto& currentItem = mq.MutableCurrentItem();
    TString url;
    TTrackUrl::EUrlFormat urlFormat;
    auto serverTimeMs = applyRequest.ServerTimeMs();
    ui64 expiringAtMs = 0;
    if (response.Defined()) {
        if (response->first == MUSIC_RESPONSE_ITEM_HLS) {
            LOG_INFO(logger) << "Building the HLS track url...";
            const auto downloadOptions = ParseDownloadInfo(response->second, logger);
            const auto urlInfo = NImpl::GetDownloadInfo(logger, applyRequest, downloadOptions);
            Y_ENSURE(urlInfo);
            url = urlInfo->DownloadInfoUrl;
            url += url.Contains('?') ? "&vsid=" : "?vsid=";
            url += MakeHollywoodVsid(rng, TInstant::MilliSeconds(serverTimeMs));
            urlFormat = TTrackUrl::UrlFormatHls;
            expiringAtMs = urlInfo->ExpiringAtMs;
        } else if (response->first == MUSIC_RESPONSE_ITEM_MP3_GET_ALICE) {
            LOG_INFO(logger) << "Building the mp3-get-alice track url...";
            const auto downloadOptions = ParseDownloadInfo(response->second, logger);
            const auto urlInfo = NImpl::GetDownloadInfo(logger, applyRequest, downloadOptions);
            Y_ENSURE(urlInfo);
            url = urlInfo->DownloadInfoUrl;
            urlFormat = TTrackUrl::UrlFormatMp3;
            expiringAtMs = urlInfo->ExpiringAtMs;
        } else {
            LOG_INFO(logger) << "Building the mp3 track url...";
            const auto xmlResp = ParseDlInfoXmlResp(response->second);
            Y_ENSURE(xmlResp);

            url = BuildTrackUrl(currentItem.GetTrackId(), mq.MakeFrom(), uid, *xmlResp);
            urlFormat = TTrackUrl::UrlFormatMp3;
        }
    } else if (mq.IsGenerative()) {
        url = currentItem.GetGenerativeInfo().GetGenerativeStreamUrl();
        urlFormat = TTrackUrl::UrlFormatHls;
    } else if (mq.IsFmRadio()) {
        url = currentItem.GetFmRadioInfo().GetFmRadioStreamUrl();
        urlFormat = TTrackUrl::UrlFormatHls;
        expiringAtMs = std::numeric_limits<decltype(expiringAtMs)>::max(); // fm radio urls never expire
    } else {
        ythrow yexception() << "Url can not be defined";
    }
    auto* trackUrlInfo = currentItem.MutableUrlInfo();
    trackUrlInfo->SetUrl(url);
    trackUrlInfo->SetUrlFormat(urlFormat);
    trackUrlInfo->SetUrlTime(serverTimeMs);
    trackUrlInfo->SetExpiringAtMs(expiringAtMs);
    *currentItem.MutablePlayId() = GenerateRandomString(rng, 12); // (26 + 26 + 10) ^ 12 > 2 ^ 64
}

ui32 GetTracksReaskDelaySecondsForCurrentTrack(const TScenarioBaseRequestWrapper& request, const TMusicQueueWrapper& mq) {
    if (mq.HasCurrentItem()) {
        const auto currentTrackDurMs = mq.CurrentItem().GetDurationMs();
        return GetTracksReaskDelaySeconds(
            request,
            std::lround(static_cast<float>(currentTrackDurMs) / 1000.0 * 0.3)
        );
    }
    return GetTracksReaskDelaySeconds(request);
}

// TODO(jan-fazli): Move to onboarding.h if ever possible, causes cyclic reference right now
void FillOnboardingTracksGameStartResponse(TScenarioHandleContext& ctx, const TScenarioApplyRequestWrapper& request,
                                           TResponseBodyBuilder& bodyBuilder, NMusic::TScenarioState& scState)
{
    auto& logger = ctx.Ctx.Logger();
    const TMusicQueueWrapper mq(logger, *scState.MutableQueue());
    auto& onboardingState = *scState.MutableOnboardingState();
    const bool masterOnboarding = onboardingState.GetInMasterOnboarding();
    LOG_INFO(logger) << TString::Join(
        "Rendering music onboarding tracks game start",
        masterOnboarding ? " for master onboarding..." : "...");

    // In master onboarding scState should already be set correctly
    if (!masterOnboarding) {
        onboardingState.SetInOnboarding(true);
        onboardingState.ClearOnboardingSequence();
        // Default 0 tracks count is infinity
        onboardingState.AddOnboardingSequence()->MutableTracksGame()->SetTracksCount(
            request.LoadValueFromExpPrefix<ui32>(NExperiments::EXP_HW_MUSIC_ONBOARDING_TRACKS_COUNT, 0u)
        );
    }

    if (request.HasExpFlag(EXP_HW_MUSIC_ONBOARDING_TRACKS_REASK)) {
        const auto& applyArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
        const auto& puid = applyArgs.GetAccountStatus().GetUid();

        auto& stage = *onboardingState.MutableOnboardingSequence(0);
        Y_ENSURE(stage.HasTracksGame());
        auto& tracksGame = *stage.MutableTracksGame();

        tracksGame.SetReaskCount(GetTracksReaskCount(request));

        auto reaskDelaySeconds = GetTracksReaskDelaySecondsForCurrentTrack(request, mq);
        bodyBuilder.AddServerDirective(
            MakeTracksMidrollDirective(request, puid, tracksGame.GetTrackIndex(), reaskDelaySeconds)
        );
    }

    TNlgData nlgData{logger, request};
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(
        TEMPLATE_MUSIC_ONBOARDING,
        masterOnboarding ? "track__game_start_master" : "track__game_start",
        /* buttons = */ {}, nlgData);
    bodyBuilder.SetExpectsRequest(true);
}

void CreateOnboardingTracksGameLikeDislikeResponse(TScenarioHandleContext& ctx, const TScenarioApplyRequestWrapper& request, TApplyResponseBuilder& builder,
                                                   NMusic::TScenarioState& scState, TMusicContext& mCtx, bool isLike = true)
{
    auto& logger = ctx.Ctx.Logger();
    const TMusicQueueWrapper mq(logger, *scState.MutableQueue());
    auto& onboardingState = *scState.MutableOnboardingState();
    const bool masterOnboarding = onboardingState.GetInMasterOnboarding();
    LOG_INFO(logger) << TString::Join(
        "Rendering music onboarding tracks game ", isLike ? "like" : "dislike",
        masterOnboarding ? " for master onboarding..." : "...");
    Y_ENSURE(onboardingState.GetInOnboarding());
    Y_ENSURE(onboardingState.OnboardingSequenceSize());

    auto& stage = *onboardingState.MutableOnboardingSequence(0);
    Y_ENSURE(stage.HasTracksGame());
    auto& tracksGame = *stage.MutableTracksGame();

    TNlgData nlgData{logger, request};
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    if (!isLike && TRepeatedSkip{scState, logger}.TryPropose(request, bodyBuilder, nlgData)) {
        // start over after a proposal (https://st.yandex-team.ru/DIALOG-7772#621cdf66d7c2ba2153162808)
        tracksGame.SetTrackIndex(0);
    } else {
        // Increment track number
        tracksGame.SetTrackIndex(tracksGame.GetTrackIndex() + 1);
    }

    const bool infTracks = tracksGame.GetTracksCount() == 0;
    const bool nextIsLast = !infTracks && tracksGame.GetTrackIndex() == (tracksGame.GetTracksCount() - 1) && tracksGame.GetTracksCount() > 1;
    const bool finished = !infTracks && tracksGame.GetTrackIndex() >= tracksGame.GetTracksCount();

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(
        TEMPLATE_MUSIC_ONBOARDING,
        finished ? "track__game_end"
        : nextIsLast ? "track__game_almost_over"
        : isLike ? "track__game_like" : "track__game_dislike",
        /* buttons = */ {}, nlgData);
    if (!finished) {
        if (request.HasExpFlag(EXP_HW_MUSIC_ONBOARDING_TRACKS_REASK)) {
            const auto& applyArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
            const auto& puid = applyArgs.GetAccountStatus().GetUid();

            tracksGame.SetReaskCount(GetTracksReaskCount(request));

            auto reaskDelaySeconds = GetTracksReaskDelaySecondsForCurrentTrack(request, mq);
            bodyBuilder.AddServerDirective(
                MakeTracksMidrollDirective(request, puid, tracksGame.GetTrackIndex(), reaskDelaySeconds)
            );
        }
        bodyBuilder.SetExpectsRequest(true);
        return;
    }

    bodyBuilder.AddListenDirective();

    mCtx.SetNeedSetPauseAtStart(true);
    scState.ClearOnboardingState();

    NScenarios::TFrameAction confirmAction;
    confirmAction.MutableNluHint()->SetFrameName(HINT_FRAME_CONFIRM);
    auto& parsed = *confirmAction.MutableParsedUtterance();
    parsed.MutableAnalytics()->SetPurpose("Play music after onboarding tracks game");
    if (isLike) {
        parsed.MutableTypedSemanticFrame()->MutablePlayerContinueSemanticFrame();
    } else {
        parsed.MutableTypedSemanticFrame()->MutablePlayerNextTrackSemanticFrame();
    }
    bodyBuilder.AddAction("music_onboarding_play_confirm", std::move(confirmAction));

    NScenarios::TFrameAction declineAction;
    declineAction.MutableNluHint()->SetFrameName(HINT_FRAME_DECLINE);
    declineAction.MutableCallback()->SetName(TString{MUSIC_ONBOARDING_ON_PLAY_DECLINE_CALLBACK});
    bodyBuilder.AddAction("music_onboarding_play_decline", std::move(declineAction));
}

NScenarios::TDirective BuildSetGlagolMetadataDirective(const TMusicQueueWrapper& mq)
{
    NScenarios::TDirective directive{};
    auto& setGlagolMetadataDirective = *directive.MutableSetGlagolMetadataDirective();

    setGlagolMetadataDirective.SetName(TString(SET_GLAGOL_METADATA_DIRECTIVE_NAME));

    TMaybe<TString> prevTrackId = Nothing();
    TMaybe<TString> nextTrackId = Nothing();
    if (mq.HasPreviousItem()) {
        prevTrackId = mq.PreviousItem().GetTrackId();
    }
    if (mq.HasNextItem()) {
        nextTrackId = mq.NextItem().GetTrackId();
    }

    FillGlagolMetadata(*setGlagolMetadataDirective.MutableGlagolMetadata(), mq.ContentId(),
                       prevTrackId, nextTrackId, mq.GetMetadataShuffled(), mq.GetMetadataRepeatType());

    return directive;
}

} // namespace

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThinClientRenderEmptyResponseHandleImpl(TScenarioHandleContext& ctx, TMusicContext& mCtx,
                                             const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                             TNlgWrapper& nlgWrapper, IRng& rng,
                                             const TShowViewBuilderSources sources,
                                             const TMusicShotsFastData& shots) {
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Rendering empty response in apply stage";
    auto& scState = *mCtx.MutableScenarioState();
    TryInitPlaybackContextBiometryOptions(logger, scState);
    TMusicQueueWrapper mq(logger, *scState.MutableQueue());

    TApplyResponseBuilder builder(&nlgWrapper, ConstructBodyRenderer(applyRequest));

    const auto& uid = mq.GetBiometryUserIdOrFallback(mCtx.GetAccountStatus().GetUid());

    FillDownloadInfoResult(ctx, applyRequest, Nothing(), mq, uid, rng);
    MakeAudioPlayDirectiveAndNextTrackCallback(ctx, applyRequest, mCtx, mq, builder, uid, sources, shots, rng);
    auto& bodyBuilder = *builder.GetResponseBodyBuilder();

    bodyBuilder.SetState(scState);
    return std::move(builder).BuildResponse();
}

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThinClientRenderGenerativeHandleImpl(TScenarioHandleContext& ctx, TMusicContext& mCtx,
                                          const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                          TNlgWrapper& nlgWrapper) {
    auto& logger = ctx.Ctx.Logger();
    auto& scState = *mCtx.MutableScenarioState();
    TryInitPlaybackContextBiometryOptions(logger, scState);
    TMusicQueueWrapper mq(logger, *scState.MutableQueue());

    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();
    const auto& playerCommand = applyArgs.GetPlayerCommand();
    TApplyResponseBuilder builder(&nlgWrapper, ConstructBodyRenderer(applyRequest));

    if (playerCommand == TMusicArguments_EPlayerCommand_Dislike) {
        LOG_INFO(logger) << "Preparing nlg for TimestampDislike command";
        const auto biometryData = ProcessBiometryOrFallback(logger, applyRequest, /* uid = */ "");
        if (!mq.IsNlgDisabled()) {
            NMusic::NImpl::RenderDislikeNlgResult(ctx, applyRequest, biometryData, builder, scState, /* isGenerative = */ true);
        }
    } else if (playerCommand == TMusicArguments_EPlayerCommand_NextTrack) {
        LOG_INFO(logger) << "Preparing nlg for TimestampSkip command";
        if (!mq.IsNlgDisabled()) {
            NMusic::NImpl::RenderTimestampSkipNlgResult(ctx, applyRequest, builder);
        }
    } else if (playerCommand == TMusicArguments_EPlayerCommand_Continue) {
        // Do nothing
    } else {
        ythrow yexception() << "Unsupported playerCommand=" << TMusicArguments_EPlayerCommand_Name(playerCommand)
                            << " in scenario apply stage for generative music";
    }

    auto& bodyBuilder = builder.GetResponseBodyBuilder() ? *builder.GetResponseBodyBuilder()
                                                         : builder.CreateResponseBodyBuilder();
    auto resetAddBuilder = bodyBuilder.ResetAddBuilder();
    resetAddBuilder.AddCallback(TString{MUSIC_THIN_CLIENT_NEXT_CALLBACK});
    CreateAndFillAnalyticsInfoForPlayerCommandWithFirstTrackObject(
        playerCommand,
        mq.CurrentItem(),
        bodyBuilder,
        applyRequest.ServerTimeMs(),
        scState.GetProductScenarioName(),
        mCtx.GetBatchOfTracksRequested(),
        mCtx.GetCacheHit());

    bodyBuilder.SetState(scState);
    return std::move(builder).BuildResponse();
}

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThinClientRenderHandleImpl(TScenarioHandleContext& ctx, TMusicContext& mCtx,
                                const TMaybeRawHttpResponse& response,
                                const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                TNlgWrapper& nlgWrapper, IRng& rng,
                                TResponseBodyBuilder::TRenderData& renderData,
                                const TShowViewBuilderSources sources,
                                const TMusicShotsFastData& shots) {
    auto& logger = ctx.Ctx.Logger();
    auto& scState = *mCtx.MutableScenarioState();
    TryInitPlaybackContextBiometryOptions(logger, scState);
    TMusicQueueWrapper mq(logger, *scState.MutableQueue());

    TApplyResponseBuilder builder(&nlgWrapper, ConstructBodyRenderer(applyRequest));

    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();

    const TCapabilityWrapper<TScenarioApplyRequestWrapper> capabilityWrapper(
        applyRequest,
        &applyArgs.GetEnvironmentState()
    );

    const auto& uid = mq.GetBiometryUserIdOrFallback(mCtx.GetAccountStatus().GetUid());

    const auto& playerCommand = applyArgs.GetPlayerCommand();
    if (playerCommand != TMusicArguments_EPlayerCommand_Dislike ||
        IsAudioPlayerPlaying(applyRequest.BaseRequestProto().GetDeviceState())) {
        FillDownloadInfoResult(ctx, applyRequest, response, mq, uid, rng);
    }

    switch (playerCommand) {
        case TMusicArguments_EPlayerCommand_Like:
            // Like in apply is only possible in onboarding tracks game
            Y_ENSURE(applyArgs.GetOnboardingTracksGame());
            CreateOnboardingTracksGameLikeDislikeResponse(ctx, applyRequest, builder, scState, mCtx, /* isLike = */ true);
            MakeAudioPlayDirectiveAndNextTrackCallbackWithFirstTrackAnalytics(
                ctx, mCtx, mq, builder, uid,
                playerCommand, applyRequest, sources,
                shots, rng
            );
            if (capabilityWrapper.HasLedDisplay()) {
                auto& bodyBuilder = *builder.GetResponseBodyBuilder();
                bodyBuilder.AddDirective(BuildDrawLedScreenDirective(GetFrontalLedImage(TMusicArguments_EPlayerCommand_Like)));
            } else if (capabilityWrapper.HasScledDisplay()) {
                auto& bodyBuilder = *builder.GetResponseBodyBuilder();
                NScledAnimation::AddStandardScled(bodyBuilder, NScledAnimation::EScledAnimations::SCLED_ANIMATION_LIKE);
                bodyBuilder.AddTtsPlayPlaceholderDirective();
            } else if (capabilityWrapper.SupportsS3Animations()) {
                const auto& animationPath = TryGetS3AnimationPath(TMusicArguments_EPlayerCommand_Like);
                if (animationPath.Defined()) {
                    auto& bodyBuilder = *builder.GetResponseBodyBuilder();
                    bodyBuilder.AddDirective(BuildDrawAnimationDirective(*animationPath));
                }
            }
            break;
        case TMusicArguments_EPlayerCommand_Dislike: {
            const bool tracksGame = applyArgs.GetOnboardingTracksGame();
            if (tracksGame) {
                CreateOnboardingTracksGameLikeDislikeResponse(ctx, applyRequest, builder, scState, mCtx, /* isLike = */ false);
            } else {
                const auto& biometryResult = applyArgs.GetBiometryResult();
                const TBiometryData biometryData{.IsIncognitoUser = biometryResult.GetIsGuestUser(), .OwnerName = biometryResult.GetOwnerName()};
                if (applyArgs.GetIsShotPlaying()) {
                    RenderShotsLikeDislikeFeedback(ctx, applyRequest, biometryData, builder, /* isLike = */ false);
                } else {
                    RenderDislikeNlgResult(ctx, applyRequest, biometryData, builder, scState);
                }
            }

            auto& bodyBuilder = *builder.GetResponseBodyBuilder();

            if (applyRequest.BaseRequestProto().GetDeviceState().GetAudioPlayer().GetPlayerState() != TDeviceState_TAudioPlayer_TPlayerState_Playing) {
                Y_ENSURE(!tracksGame); // Onboarding tracks game is only possible with audio player playing
                CreateAndFillAnalyticsInfoForPlayerCommand(playerCommand, bodyBuilder, applyRequest.ServerTimeMs(),
                                                           scState.GetProductScenarioName(),
                                                           /* isMusicPlaying = */ false,
                                                           mCtx.GetBatchOfTracksRequested(),
                                                           mCtx.GetCacheHit());
                break;
            }

            if (auto shot = mq.GetShotBeforeCurrentItem()) {
                CreateAndFillAnalyticsInfoForPlayerCommandWithFirstTrackObject(
                    playerCommand, mq.CurrentItem(), bodyBuilder, applyRequest.ServerTimeMs(),
                    scState.GetProductScenarioName(), mCtx.GetBatchOfTracksRequested(), mCtx.GetCacheHit());
                LOG_INFO(logger) << "Rendering shot response";
                MakeShotDirectiveAndNextTrackCallback(ctx, *shot, uid, builder, mCtx, mq);
            } else if (mq.HasCurrentItem() || mq.GetRepeatType() != NMusic::RepeatNone) {
                // TODO(vitvlkv): Dislike should actually exit the repeat mode, see https://st.yandex-team.ru/HOLLYWOOD-161
                if (capabilityWrapper.HasLedDisplay()) {
                    bodyBuilder.AddDirective(BuildDrawLedScreenDirective(GetFrontalLedImage(TMusicArguments_EPlayerCommand_Dislike)));
                } else if (capabilityWrapper.HasScledDisplay()) {
                    NScledAnimation::AddStandardScled(bodyBuilder, NScledAnimation::EScledAnimations::SCLED_ANIMATION_DISLIKE);
                    bodyBuilder.AddTtsPlayPlaceholderDirective();
                } else if (capabilityWrapper.SupportsS3Animations()) {
                    const auto animationPath = TryGetS3AnimationPath(TMusicArguments_EPlayerCommand_Dislike);
                    if (animationPath.Defined()) {
                        auto& bodyBuilder = *builder.GetResponseBodyBuilder();
                        bodyBuilder.AddDirective(BuildDrawAnimationDirective(*animationPath));
                        bodyBuilder.AddTtsPlayPlaceholderDirective();
                    }
                }
                MakeAudioPlayDirectiveAndNextTrackCallbackWithFirstTrackAnalytics(
                    ctx, mCtx, mq, builder, uid,
                    playerCommand, applyRequest, sources,
                    shots, rng
                );
            } else if (!mq.IsAutoflowDisabled()) {
                Y_ENSURE(!tracksGame);
                // else we should turn on the radio. We use ResetAdd here to resolve HOLLYWOOD-267 analytics problem
                auto resetAddBuilder = bodyBuilder.ResetAddBuilder();
                resetAddBuilder.AddCallback(TString{MUSIC_THIN_CLIENT_NEXT_CALLBACK});
                resetAddBuilder.AddRecoveryActionCallback(TString{MUSIC_THIN_CLIENT_RECOVERY_CALLBACK},
                                                          mq.MakeRecoveryActionCallbackPayload());

                CreateAndFillAnalyticsInfoForPlayerCommandBeforeAutoflow(playerCommand, mq.CurrentItem(),
                                                                         bodyBuilder, applyRequest.ServerTimeMs(),
                                                                         scState.GetProductScenarioName(),
                                                                         mCtx.GetBatchOfTracksRequested(),
                                                                         mCtx.GetCacheHit());
            } else {
                bodyBuilder.AddClientActionDirective("clear_queue", {});
                auto resetAddBuilder = bodyBuilder.ResetAddBuilder();

                // TODO(lavv17): use empty ResetAdd after MEGAMIND-3441
                // For now, force stack update by DoNothing
                auto& utterance = resetAddBuilder.AddUtterance({});
                auto& analytics = *utterance.MutableAnalytics();
                analytics.SetPurpose("do_nothing");
                analytics.SetOrigin(TAnalyticsTrackingModule::Scenario);
                utterance.MutableTypedSemanticFrame()->MutableDoNothingSemanticFrame();

                CreateAndFillAnalyticsInfoForPlayerCommand(playerCommand, bodyBuilder, applyRequest.ServerTimeMs(),
                                                           scState.GetProductScenarioName(),
                                                           /* isMusicPlaying = */ false,
                                                           mCtx.GetBatchOfTracksRequested(),
                                                           mCtx.GetCacheHit());
            }
            break;
        }
        case TMusicArguments_EPlayerCommand_None: {
            // NOTE: Callback music_thin_client_next goes here too (without FirstPlay flag of course)
            TMultiroomTokenWrapper multiroomToken{scState};
            if (mCtx.GetFirstPlay()) {
                SetBedtimeTalesState(applyRequest, scState, applyArgs.GetFairyTaleArguments().GetIsBedtimeTales());

                auto extraInfo = MakeExtraContentInfo(ctx, mCtx, applyRequest, rng);
                TryMakeMultiroomDirectives(applyRequest, extraInfo, builder, multiroomToken);
                RenderMusicNlgResult(ctx, mq, extraInfo, applyRequest, builder, nullptr);

                auto& bodyBuilder = *builder.GetResponseBodyBuilder();
                CreateAndFillAnalyticsInfoForMusicPlay(logger, applyRequest, mq, applyArgs, bodyBuilder,
                    scState.GetProductScenarioName(), /* onboardingTracksGame = */ false,
                    mCtx.GetBatchOfTracksRequested(), mCtx.GetCacheHit());
            }
            MakeAudioPlayDirectiveAndNextTrackCallback(ctx, applyRequest, mCtx, mq, builder, uid,
                                                       sources, shots, rng,
                                                       &multiroomToken);
            break;
        }
        default:
            ythrow yexception() << "Unsupported player command in Apply: "
                                << TMusicArguments_EPlayerCommand_Name(playerCommand);
    }
    auto& bodyBuilder = *builder.GetResponseBodyBuilder();
    bodyBuilder.SetState(scState);
    renderData = bodyBuilder.MoveRenderData();

    return std::move(builder).BuildResponse();
}

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ContinueThinClientRenderHandleImpl(TScenarioHandleContext& ctx, TMusicContext& mCtx,
                                   const TMaybeRawHttpResponse& response,
                                   const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                   TNlgWrapper& nlgWrapper, IRng& rng, const TMusicFastData* fastData,
                                   TResponseBodyBuilder::TRenderData& renderData,
                                   const TShowViewBuilderSources sources,
                                   const TMusicShotsFastData& shots) {
    auto& logger = ctx.Ctx.Logger();
    auto& scState = *mCtx.MutableScenarioState();
    TryInitPlaybackContextBiometryOptions(logger, scState);
    TMusicQueueWrapper mq(logger, *scState.MutableQueue());

    TApplyResponseBuilder builder(&nlgWrapper, ConstructBodyRenderer(applyRequest));

    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();

    const auto& uid = mq.GetBiometryUserIdOrFallback(mCtx.GetAccountStatus().GetUid());

    FillDownloadInfoResult(ctx, applyRequest, response, mq, uid, rng);

    const auto playerCommand = applyArgs.GetPlayerCommand();
    switch (playerCommand) {
        case TMusicArguments_EPlayerCommand_Shuffle: {
            Y_ENSURE(!mq.IsRadio(), "Shuffle command for radio must be handled on run scenario stage");
            RenderShuffleNlgResult(ctx, applyRequest, builder, /* isRadio = */ false);
            auto& bodyBuilder = *builder.GetResponseBodyBuilder();
            CreateAndFillAnalyticsInfoForPlayerCommand(playerCommand, bodyBuilder, applyRequest.ServerTimeMs(),
                                                       scState.GetProductScenarioName(),
                                                       /* isMusicPlaying = */ true, mCtx.GetBatchOfTracksRequested(),
                                                       mCtx.GetCacheHit());
            bodyBuilder.AddDirective(BuildSetGlagolMetadataDirective(mq));
            break;
        }
        case TMusicArguments_EPlayerCommand_Unshuffle: {
            Y_ENSURE(!mq.IsRadio(), "Shuffle command for radio must be handled on run scenario stage");
            auto& bodyBuilder = builder.CreateResponseBodyBuilder();
            CreateAndFillAnalyticsInfoForPlayerCommand(playerCommand, bodyBuilder, applyRequest.ServerTimeMs(),
                                                       scState.GetProductScenarioName(),
                                                       /* isMusicPlaying = */ true, mCtx.GetBatchOfTracksRequested(),
                                                       mCtx.GetCacheHit());
            bodyBuilder.AddDirective(BuildSetGlagolMetadataDirective(mq));
            break;
        }
        case TMusicArguments_EPlayerCommand_NextTrack:
            [[fallthrough]];
        case TMusicArguments_EPlayerCommand_PrevTrack:
            [[fallthrough]];
        case TMusicArguments_EPlayerCommand_ChangeTrackNumber:
            [[fallthrough]];
        case TMusicArguments_EPlayerCommand_Continue:
            [[fallthrough]];
        case TMusicArguments_EPlayerCommand_ChangeTrackVersion:
            [[fallthrough]];
        case TMusicArguments_EPlayerCommand_Replay: {

            if (playerCommand == TMusicArguments_EPlayerCommand_NextTrack) {
                RenderNextTrackNlgResult(ctx, applyRequest, builder, scState);
            } else if (playerCommand == TMusicArguments_EPlayerCommand_ChangeTrackVersion) {
                auto extraInfo = MakeExtraContentInfo(ctx, mCtx, applyRequest, rng);
                RenderMusicNlgResult(ctx, mq, extraInfo, applyRequest, builder, fastData);
            }

            auto& bodyBuilder = builder.GetOrCreateResponseBodyBuilder();
            CreateAndFillAnalyticsInfoForPlayerCommandWithFirstTrackObject(
                playerCommand,
                mq.CurrentItem(),
                bodyBuilder,
                applyRequest.ServerTimeMs(),
                scState.GetProductScenarioName(), mCtx.GetBatchOfTracksRequested(), mCtx.GetCacheHit());
            if (playerCommand != TMusicArguments_EPlayerCommand_Replay) {
                const TCapabilityWrapper<TScenarioApplyRequestWrapper> capabilityWrapper(
                    applyRequest,
                    &applyArgs.GetEnvironmentState()
                );
                if (capabilityWrapper.HasLedDisplay() && CommandSupportsFrontalLedImage(playerCommand)) {
                    bodyBuilder.AddDirective(BuildDrawLedScreenDirective(GetFrontalLedImage(playerCommand)));
                } else if (capabilityWrapper.HasScledDisplay()) {
                    //
                    // Bug: https://st.yandex-team.ru/ALICE-14148
                    //
                    // Send SCLED directive in case if source request contains voice
                    // Don't send SCLED directive in case if source request was made by swipe gesture
                    //
                    if (applyRequest.Input().IsVoiceInput()) {
                        if (playerCommand == TMusicArguments_EPlayerCommand_NextTrack) {
                            NScledAnimation::AddStandardScled(bodyBuilder, NScledAnimation::EScledAnimations::SCLED_ANIMATION_NEXT);
                            bodyBuilder.AddTtsPlayPlaceholderDirective();
                        } else if (playerCommand == TMusicArguments_EPlayerCommand_PrevTrack) {
                            NScledAnimation::AddStandardScled(bodyBuilder, NScledAnimation::EScledAnimations::SCLED_ANIMATION_PREVIOUS);
                            bodyBuilder.AddTtsPlayPlaceholderDirective();
                        }
                    }
                } else if (capabilityWrapper.SupportsS3Animations() && applyRequest.Input().IsVoiceInput()) {
                    const auto animationPath = TryGetS3AnimationPath(playerCommand);
                    if (animationPath.Defined()) {
                        auto& bodyBuilder = *builder.GetResponseBodyBuilder();
                        bodyBuilder.AddDirective(BuildDrawAnimationDirective(*animationPath));
                    }
                }
            }

            if (auto shot = mq.GetShotBeforeCurrentItem()) {
                LOG_INFO(logger) << "Rendering shot response";
                MakeShotDirectiveAndNextTrackCallback(ctx, *shot, uid, builder, mCtx, mq);
            } else {
                MakeAudioPlayDirectiveAndNextTrackCallback(ctx, applyRequest, mCtx, mq, builder, uid,
                                                           sources, shots, rng);
            }
            break;
        }
        case TMusicArguments_EPlayerCommand_None: {
            // NOTE: Callback music_thin_client_next goes here too (without FirstPlay flag of course)
            TMaybe<TString> soundFrameName =
                NSound::RenderSoundChangeIfExists(logger, applyRequest, applyRequest.Input(), builder);

            SetBedtimeTalesState(applyRequest, scState, applyArgs.GetFairyTaleArguments().GetIsBedtimeTales());

            // We do not check scState.GetOnboardingState().GetInOnboarding() here, cause scState is lost after run stage
            const bool tracksGame = applyArgs.GetOnboardingTracksGame();
            auto extraInfo = MakeExtraContentInfo(ctx, mCtx, applyRequest, rng);

            bool isGenerativeCallback = false;
            if (const auto* callback = applyRequest.Input().GetCallback()) {
                if (IsNextTrackCallback(callback) && mq.IsGenerative()) {
                    isGenerativeCallback = true;
                }
            }

            TMultiroomTokenWrapper multiroomToken{scState};
            if (!isGenerativeCallback && !tracksGame && mCtx.GetFirstPlay()) {
                TryMakeMultiroomDirectives(applyRequest, extraInfo, builder, multiroomToken);
                RenderMusicNlgResult(ctx, mq, extraInfo, applyRequest, builder, fastData);
            }
            auto& bodyBuilder = builder.GetResponseBodyBuilder() ? *builder.GetResponseBodyBuilder()
                                                                    : builder.CreateResponseBodyBuilder();
            if (tracksGame) {
                FillOnboardingTracksGameStartResponse(ctx, applyRequest, bodyBuilder, scState);
            }

            if (mCtx.GetFirstPlay()) {
                CreateAndFillAnalyticsInfoForMusicPlay(logger, applyRequest, mq, applyArgs, bodyBuilder,
                             scState.GetProductScenarioName(), tracksGame,
                                mCtx.GetBatchOfTracksRequested(), mCtx.GetCacheHit());
            } else {
                CreateAndFillAnalyticsInfoForNextPlayCallback(logger, mq, bodyBuilder);
            }

            // Directives should be added after RenderMusicNlgResult() call because RenderMusicNlgResult()
            // creates a separate TResponseBodyBuilder instance from BASS form.
            // And also after analytics, so that an analytics action can be safely added here.
            if (extraInfo.ShouldAddChildAgePromo) {
                AddChildAgePromoDirectives(logger, applyRequest, bodyBuilder);
            }
            if (extraInfo.ShouldAddBedtimeTalesOnboarding) {
                AddBedtimeTalesOnboardingDirective(logger, applyRequest, bodyBuilder);
            }

            if (!mCtx.GetFirstPlay() && CheckFairyTalesStopTimer(applyRequest, scState)) {
                AddPlayerStopDirectives(builder);
                scState.ClearFairytaleTurnOffTimer();
            }
            else if (auto shot = mq.GetShotBeforeCurrentItem()) {
                LOG_INFO(logger) << "Rendering shot response";
                MakeShotDirectiveAndNextTrackCallback(ctx, *shot, uid, builder, mCtx, mq);
            } else {
                MakeAudioPlayDirectiveAndNextTrackCallback(ctx, applyRequest, mCtx, mq, builder, uid,
                                                           sources, shots, rng,
                                                           &multiroomToken);
            }

            if (applyArgs.GetFairyTaleArguments().GetIsFairyTaleSubscenario()) {
                scState.SetProductScenarioName(MUSIC_FAIRYTALE_SCENARIO_NAME);
            } else {
                scState.ClearProductScenarioName();
            }

            NSound::FillAnalyticsInfoForSoundChangeIfExists(soundFrameName, bodyBuilder);
            break;
        }
        default:
            ythrow yexception() << "Unsupported player command in Continue: "
                                << TMusicArguments_EPlayerCommand_Name(playerCommand);
    }
    auto& bodyBuilder = *builder.GetResponseBodyBuilder();
    bodyBuilder.SetState(scState);
    renderData = bodyBuilder.MoveRenderData();

    return std::move(builder).BuildResponse();
}

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThinClientRenderNonPremiumHandleImpl(TScenarioHandleContext& ctx, TMusicContext& mCtx,
                                          const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                          TNlgWrapper& nlgWrapper) {
    auto& logger = ctx.Ctx.Logger();
    TApplyResponseBuilder builder(&nlgWrapper, ConstructBodyRenderer(applyRequest, /* forceNlg = */ true));
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    TryAddUnauthorizedUserDirectivesForThinClient(applyRequest, mCtx, bodyBuilder);

    TNlgData nlgData{logger, applyRequest};
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, RENDER_SUBSCRIPTION_REQUIRED, {}, nlgData);

    bodyBuilder.SetState(mCtx.GetScenarioState());

    auto& analyticsInfoBuilder = bodyBuilder.CreateAnalyticsInfoBuilder();
    FillAnalyticsInfoForMusicPlaySimple(analyticsInfoBuilder);

    return std::move(builder).BuildResponse();
}

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThinClientRenderUnauthorizedHandleImpl(TScenarioHandleContext& ctx, TMusicContext& mCtx,
                                            const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                            TNlgWrapper& nlgWrapper) {
    auto& logger = ctx.Ctx.Logger();
    TApplyResponseBuilder builder(&nlgWrapper, ConstructBodyRenderer(applyRequest, /* forceNlg = */ true));
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    TryAddUnauthorizedUserDirectivesForThinClient(applyRequest, mCtx, bodyBuilder);

    TNlgData nlgData{logger, applyRequest};
    nlgData.Context["error"]["data"]["code"] = "music_authorization_problem";
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, RENDER_UNAUTHORIZED, {}, nlgData);

    bodyBuilder.SetState(mCtx.GetScenarioState());

    return std::move(builder).BuildResponse();
}

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThinClientRenderErrorHandleImpl(TScenarioHandleContext& ctx, const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                     TNlgWrapper& nlgWrapper, const TMusicContext& mCtx) {
    auto& logger = ctx.Ctx.Logger();
    const TMusicContext::TContentStatus contentStatus = mCtx.GetContentStatus();
    TApplyResponseBuilder builder(&nlgWrapper, ConstructBodyRenderer(applyRequest, /* forceNlg = */ true));
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    TNlgData nlgData{logger, applyRequest};

    TStringBuf errorType;
    if (contentStatus.GetErrorVer2() != NMusic::NoError) {
        errorType = RENDER_MUSIC_ERROR;
        nlgData.Context["attentions"].SetType(NJson::JSON_MAP);

        TStringBuf errorCode;
        switch (contentStatus.GetErrorVer2()) {
            case ErrorForbiddenVer2:
                errorCode = TStringBuf("forbidden-content");
                break;
            case ErrorRestrictedByChildVer2:
                errorCode = TStringBuf("music_restricted_by_child_content_settings");
                if (contentStatus.GetAttentionVer2() == NMusic::AttentionForbiddenPodcast) {
                    nlgData.Context["attentions"]["forbidden_podcast"] = true;
                }
                break;
            case ErrorNotFoundVer2:
                errorCode = TStringBuf("music_not_found");
                break;
            case NoError:
            case EContentErrorVer2_INT_MIN_SENTINEL_DO_NOT_USE_:
            case EContentErrorVer2_INT_MAX_SENTINEL_DO_NOT_USE_:
                // Exhaust enum values to prevent warning
                break;
        }
        nlgData.Context["error"]["data"]["code"] = errorCode;
    } else {
        errorType = RENDER_GENERIC_ERROR;
    }

    NMusic::TScenarioState scState;
    const bool hasState = ReadScenarioState(applyRequest.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(logger, scState);

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, errorType, {}, nlgData);
    if (hasState) {
        bodyBuilder.SetState(scState);
    }

    if (applyRequest.Interfaces().GetHasLedDisplay()) {
        auto& bodyBuilder = *builder.GetResponseBodyBuilder();
        bodyBuilder.AddDirective(BuildDrawLedScreenDirective(PLAYER_ERROR_GIF_URI));
    }

    return std::move(builder).BuildResponse();
}

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThinClientRenderPromoHandleImpl(TScenarioHandleContext& ctx, TMusicContext& mCtx,
                                     const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                     TNlgWrapper& nlgWrapper)
{
    auto& logger = ctx.Ctx.Logger();
    Y_ENSURE(mCtx.HasAccountStatus() && !mCtx.GetAccountStatus().GetHasMusicSubscription() &&
             mCtx.GetAccountStatus().HasPromo());

    TApplyResponseBuilder builder(&nlgWrapper, ConstructBodyRenderer(applyRequest));
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    TryAddUnauthorizedUserDirectivesForThinClient(applyRequest, mCtx, bodyBuilder);

    TNlgData nlgData{logger, applyRequest};
    if (mCtx.GetAccountStatus().GetPromo().GetExtraPeriodExpiresDate().empty()) {
        nlgData.Context["error"]["data"]["code"] = TStringBuf(NAlice::NMusic::ERROR_CODE_PROMO_AVAILABLE);
    } else {
        nlgData.Context["error"]["data"]["code"] = TStringBuf(NAlice::NMusic::ERROR_CODE_EXTRA_PROMO_PERIOD_AVAILABLE);
        nlgData.Context[NAlice::NBilling::EXTRA_PROMO_PERIOD_EXPIRES_DATE] = mCtx.GetAccountStatus().GetPromo().GetExtraPeriodExpiresDate();
    }

    nlgData.ReqInfo["experiments"].SetType(NJson::JSON_MAP);
    LOG_INFO(logger) << "Constructed nlg context=" << nlgData.Context;

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, RENDER_UNAUTHORIZED, {}, nlgData);

    NScenarios::TServerDirective directive;
    auto& pushDirective = *directive.MutablePushMessageDirective();
    pushDirective.SetTitle(PROMO_TITLE);
    pushDirective.SetBody(PROMO_BODY);
    pushDirective.SetLink(mCtx.GetAccountStatus().GetPromo().GetActivatePromoUri());
    pushDirective.SetPushId("alice_quasar_promo_period_yandexplus");
    const auto uid = mCtx.GetAccountStatus().GetUid(); // TODO(klim-roma): consider handling guest mode request
    pushDirective.SetPushTag(TStringBuilder() << "QUASAR.PROMO_PERIOD.yandexplus." << uid);
    pushDirective.SetThrottlePolicy("quasar_default_install_id");
    pushDirective.AddAppTypes(NAlice::EAppType::AT_SEARCH_APP);
    bodyBuilder.AddServerDirective(std::move(directive));

    bodyBuilder.SetState(mCtx.GetScenarioState());

    return std::move(builder).BuildResponse();
}

std::unique_ptr<NAlice::NScenarios::TScenarioRunResponse>
RunThinClientRenderGenerativeUnsupportedPlayerCommand(
    const NMusic::TScenarioState& scState,
    TScenarioHandleContext& ctx,
    const NHollywood::TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    const TStringBuf nlgTemplateName,
    TMusicArguments_EPlayerCommand unsupportedPlayerCommand)
{
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Rendering " << nlgTemplateName << " NLG...";
    THwFrameworkRunResponseBuilder builder(ctx, &nlg, ConstructBodyRenderer(request));
    auto& bodyBuilder = builder.GetResponseBodyBuilder() ? *builder.GetResponseBodyBuilder()
                                                         : builder.CreateResponseBodyBuilder();
    TNlgData nlgData{logger, request};
    nlgData.Context["is_generative"] = true;
    nlgData.Context["attentions"].SetType(NJson::JSON_MAP);
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(nlgTemplateName, "render_result", {}, nlgData);
    bodyBuilder.SetState(scState);

    CreateAndFillAnalyticsInfoForPlayerCommand(unsupportedPlayerCommand, bodyBuilder, request.ServerTimeMs(),
                                               scState.GetProductScenarioName());

    NMusic::FillPlayerFeatures(logger, request, builder);
    return std::move(builder).BuildResponse();
}

std::unique_ptr<NAlice::NScenarios::TScenarioRunResponse>
RunThinClientRenderFmRadioUnsupportedPlayerCommand(
    const TMusicQueueWrapper& mq,
    const NMusic::TScenarioState& scState,
    TScenarioHandleContext& ctx,
    const NHollywood::TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    const TStringBuf nlgTemplateName,
    TMusicArguments_EPlayerCommand unsupportedPlayerCommand)
{
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Rendering " << nlgTemplateName << " NLG...";
    THwFrameworkRunResponseBuilder builder(ctx, &nlg, ConstructBodyRenderer(request));
    auto& bodyBuilder = builder.GetResponseBodyBuilder() ? *builder.GetResponseBodyBuilder()
                                                         : builder.CreateResponseBodyBuilder();
    TNlgData nlgData{logger, request};
    nlgData.Context["is_fm_radio"] = true;
    Y_ENSURE(mq.HasCurrentItem());
    nlgData.Context["fm_radio_name"] = mq.CurrentItem().GetTitle();
    nlgData.Context["attentions"].SetType(NJson::JSON_MAP);
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(nlgTemplateName, "render_result", {}, nlgData);
    bodyBuilder.SetState(scState);

    CreateAndFillAnalyticsInfoForPlayerCommand(unsupportedPlayerCommand, bodyBuilder, request.ServerTimeMs(),
                                               scState.GetProductScenarioName());

    NMusic::FillPlayerFeatures(logger, request, builder);
    return std::move(builder).BuildResponse();
}

std::unique_ptr<NAlice::NScenarios::TScenarioRunResponse>
RunThinClientRenderUnknownMusicPlayerCommand(
    TScenarioHandleContext& ctx,
    const NHollywood::TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    const TStringBuf nlgTemplateName)
{
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Rendering " << nlgTemplateName << " NLG...";
    THwFrameworkRunResponseBuilder builder(ctx, &nlg, ConstructBodyRenderer(request));
    auto& bodyBuilder = builder.GetResponseBodyBuilder() ? *builder.GetResponseBodyBuilder()
                                                         : builder.CreateResponseBodyBuilder();
    TNlgData nlgData{logger, request};
    nlgData.AddAttention("unknown_music");
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(nlgTemplateName, "render_result", {}, nlgData);

    if (NMusic::TScenarioState scState; ReadScenarioState(request.BaseRequestProto(), scState)) {
        TryInitPlaybackContextBiometryOptions(logger, scState);
        bodyBuilder.SetState(scState);
    }

    NMusic::FillPlayerFeatures(logger, request, builder);
    return std::move(builder).BuildResponse();
}

void RenderLikeNlgResult(TScenarioHandleContext& ctx, const NHollywood::TScenarioRunRequestWrapper& runRequest,
                         const TBiometryData& biometryData, THwFrameworkRunResponseBuilder& builder) {
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Rendering like render_result NLG...";
    auto& bodyBuilder = builder.GetResponseBodyBuilder() ? *builder.GetResponseBodyBuilder()
                                                         : builder.CreateResponseBodyBuilder();

    TNlgData nlgData{logger, runRequest};
    nlgData.Context["attentions"]["biometry_guest"] = biometryData.IsIncognitoUser;
    nlgData.Context["user_name"] = biometryData.OwnerName;
    LOG_INFO(logger) << "Constructed nlg context=" << nlgData.Context;

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_PLAYER_LIKE, "render_result", {}, nlgData);
}

void RenderDislikeNlgResult(TScenarioHandleContext& ctx, const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                            const TBiometryData& biometryData, TApplyResponseBuilder& builder,
                            NMusic::TScenarioState& scState, bool isGenerative) {
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Rendering dislike render_result NLG...";
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    TNlgData nlgData{logger, applyRequest};
    nlgData.Context["attentions"]["biometry_guest"] = biometryData.IsIncognitoUser;
    nlgData.Context["user_name"] = biometryData.OwnerName;
    nlgData.Context["is_generative"] = isGenerative;
    TRepeatedSkip{scState, logger}.TryPropose(applyRequest, bodyBuilder, nlgData);
    LOG_INFO(logger) << "Constructed nlg context=" << nlgData.Context;

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_PLAYER_DISLIKE, "render_result", {}, nlgData);
}

void RenderTimestampSkipNlgResult(TScenarioHandleContext& ctx,
                                  const NHollywood::TScenarioApplyRequestWrapper& applyRequest, TApplyResponseBuilder& builder) {
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Rendering timestamp_skip render result NLG...";
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    TNlgData nlgData{logger, applyRequest};
    nlgData.Context["is_generative"] = true;
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_PLAYER_TIMESTAMP_SKIP, "render_result", {}, nlgData);
}

void RenderNoPrevTrackNlgResult(TScenarioHandleContext& ctx, const NHollywood::TScenarioRunRequestWrapper& runRequest,
                                THwFrameworkRunResponseBuilder& builder)
{
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Rendering previous_track NLG...";
    auto& bodyBuilder = builder.GetResponseBodyBuilder() ? *builder.GetResponseBodyBuilder()
                                                         : builder.CreateResponseBodyBuilder();
    TNlgData nlgData{logger, runRequest};
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_PLAYER_PREV_TRACK, "render_result", {}, nlgData);
}

} // namespace NImpl

} // namespace NAlice::NHollywood::NMusic
