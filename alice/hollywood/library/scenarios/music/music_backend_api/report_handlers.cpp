#include "report_handlers.h"
#include "music_common.h"

#include <alice/hollywood/library/scenarios/music/biometry/process_biometry.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/consts.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/content_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/play_audio/play_audio.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/shots.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/library/experiments/flags.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr auto MIN_PLAY_TIME_SEC = 0.1f;
constexpr TStringBuf PLAY_AUDIO_EVENT = "playAudioEvent";
constexpr TStringBuf GENERATIVE_FEEDBACK_EVENT = "generativeFeedbackEvent";
const TString GENERATIVE_FEEDBACK_TYPE_STREAM_STARTED = "streamStarted";
const TString GENERATIVE_FEEDBACK_TYPE_STREAM_PLAY = "streamPlay";
const TString GENERATIVE_FEEDBACK_TYPE_STREAM_PAUSE = "streamPause";
constexpr TStringBuf RADIO_FEEDBACK_EVENT = "radioFeedbackEvent";
const TString RADIO_FEEDBACK_TYPE_RADIO_STARTED = "radioStarted";
const TString SHOT_FEEDBACK_EVENT = "shotFeedbackEvent";
const TString RADIO_FEEDBACK_TYPE_TRACK_STARTED = "trackStarted";
const TString RADIO_FEEDBACK_TYPE_SKIP = "skip";
const TString RADIO_FEEDBACK_TYPE_TRACK_FINISHED = "trackFinished";
const TString RADIO_FEEDBACK_TYPE_DISLIKE = "dislike";
const TString RADIO_FEEDBACK_TYPE_LIKE = "like";
constexpr TStringBuf RADIO_FEEDBACK_PUMPKIN_RADIO_SESSION_ID = "<PUMPKIN>";

TString ConvertGenerativeFeedbackType(const TString& type) {
    if (type == "StreamStarted") {
        return GENERATIVE_FEEDBACK_TYPE_STREAM_STARTED;
    } else if (type == "StreamPlay") {
        return GENERATIVE_FEEDBACK_TYPE_STREAM_PLAY;
    } else if (type == "StreamPause") {
        return GENERATIVE_FEEDBACK_TYPE_STREAM_PAUSE;
    } else if (type == "TimestampLike") {
        return GENERATIVE_FEEDBACK_TYPE_TIMESTAMP_LIKE;
    } else if (type == "TimestampDislike") {
        return GENERATIVE_FEEDBACK_TYPE_TIMESTAMP_DISLIKE;
    } else if (type == "TimestampSkip") {
        return GENERATIVE_FEEDBACK_TYPE_TIMESTAMP_SKIP;
    }
    ythrow yexception() << "Unsupported generative feedback type, " << type;
}

TString ConvertRadioFeedbackType(const TString& type) {
    if (type == "RadioStarted") {
        return RADIO_FEEDBACK_TYPE_RADIO_STARTED;
    } else if (type == "TrackStarted") {
        return RADIO_FEEDBACK_TYPE_TRACK_STARTED;
    } else if (type == "Skip") {
        return RADIO_FEEDBACK_TYPE_SKIP;
    } else if (type == "TrackFinished") {
        return RADIO_FEEDBACK_TYPE_TRACK_FINISHED;
    } else if (type == "Dislike") {
        return RADIO_FEEDBACK_TYPE_DISLIKE;
    }
    ythrow yexception() << "Unsupported radio feedback type, " << type;
}

NAppHostHttp::THttpRequest PreparePlayAudioProxyRequest(const NJson::TJsonValue& playAudioJson, TInstant timestamp,
                                               float playedSec, float offsetSec, float durationSec,
                                               const NScenarios::TRequestMeta& meta,
                                               const TClientInfo& clientInfo,
                                               const bool enableCrossDc, const TStringBuf ownerUserId,
                                               ERequestMode requestMode, TRTLogger& logger) {
    LOG_INFO(logger) << "Preparing play audio proxy request";
    const auto payloadJsonStr = TPlayAudioJsonBuilder(playAudioJson)
        .Timestamp(timestamp)
        .PlayedSec(playedSec)
        .PositionSec(offsetSec)
        .StartPositionSec(offsetSec - playedSec)
        .DurationSec(durationSec)
        .BuildJsonString();

    const NJson::TJsonValue* userIdPtr = nullptr;
    playAudioJson.GetValuePointer("uid", &userIdPtr);
    Y_ENSURE(userIdPtr);
    const TString userId = userIdPtr->GetStringSafe("");
    Y_ENSURE(!userId.Empty());

    auto musicRequestModeInfo = TMusicRequestModeInfoBuilder()
                            .SetAuthMethod(EAuthMethod::UserId)
                            .SetRequestMode(requestMode)
                            .SetOwnerUserId(ownerUserId)
                            .SetRequesterUserId(userId)
                            .BuildAndMove();

    return TMusicRequestBuilder(NApiPath::PlayAudioPlays(/* clientNow = */ {}, /* userId = */ userId),
                                meta, clientInfo, logger, enableCrossDc, musicRequestModeInfo, "PlayAudio")
        .SetMethod(NAppHostHttp::THttpRequest_EMethod_Post)
        .SetBody(payloadJsonStr)
        .Build(/* logVerbose = */ true);
}

NAppHostHttp::THttpRequest PrepareSaveProgressRequest(const NJson::TJsonValue& playAudioJson, TInstant timestamp,
                                             float offsetSec, float durationSec,
                                             const NScenarios::TRequestMeta& meta,
                                             const TClientInfo& clientInfo,
                                             const bool enableCrossDc, const TStringBuf ownerUserId,
                                             ERequestMode requestMode, TRTLogger& logger) {
    LOG_INFO(logger) << "Preparing save stream progress proxy request";

    const NJson::TJsonValue* userIdPtr = nullptr;
    playAudioJson.GetValuePointer("uid", &userIdPtr);
    Y_ENSURE(userIdPtr);
    const TString userId = userIdPtr->GetStringSafe("");
    Y_ENSURE(!userId.Empty());

    auto musicRequestModeInfo = TMusicRequestModeInfoBuilder()
                            .SetAuthMethod(EAuthMethod::UserId)
                            .SetRequestMode(requestMode)
                            .SetOwnerUserId(ownerUserId)
                            .SetRequesterUserId(userId)
                            .BuildAndMove();

    return TMusicRequestBuilder(NApiPath::SaveProgress(playAudioJson["trackId"].GetString(), offsetSec, durationSec,
                                                       timestamp, /* clientNow = */ {}, /* userId = */ userId),
                                meta, clientInfo, logger, enableCrossDc, musicRequestModeInfo, "SaveProgress")
        .SetMethod(NAppHostHttp::THttpRequest_EMethod_Post)
        .Build();
}

NAppHostHttp::THttpRequest PrepareGenerativeFeedbackProxyRequest(
    const NJson::TJsonValue& generativeFeedbackJson, TInstant timestamp,
    const NScenarios::TRequestMeta& meta, const TClientInfo& clientInfo,
    const bool enableCrossDc, TRTLogger& logger)
{
    LOG_INFO(logger) << "Preparing generative feedback proxy request";
    const auto& type = ConvertGenerativeFeedbackType(generativeFeedbackJson["type"].GetStringSafe());
    NJson::TJsonValue payloadJson;
    payloadJson["type"] = type;
    payloadJson["timestamp"] = timestamp.ToStringUpToSeconds();

    auto requestItemName = TString(MUSIC_GENERATIVE_FEEDBACK_REQUEST_ITEM);
    auto payloadJsonStr = JsonToString(payloadJson);
    const auto generativeStationId = generativeFeedbackJson["generativeStationId"].GetStringSafe();
    const auto streamId = generativeFeedbackJson["streamId"].GetStringSafe();
    Y_ENSURE(!generativeStationId.Empty());

    auto musicRequestModeInfoBuilder = TMusicRequestModeInfoBuilder().SetAuthMethod(EAuthMethod::OAuth);
    auto guestOAuthTokenEncrypted = generativeFeedbackJson["guestOAuthTokenEncrypted"].GetStringSafe("");
    TAtomicSharedPtr<IRequestMetaProvider> metaProvider;
    if (!guestOAuthTokenEncrypted.Empty()) {
        metaProvider = MakeGuestRequestMetaProvider(meta, guestOAuthTokenEncrypted);
        musicRequestModeInfoBuilder.SetRequestMode(ERequestMode::ClientBiometry);
    } else {
        metaProvider = MakeAtomicShared<TRequestMetaProvider>(meta);
        musicRequestModeInfoBuilder.SetRequestMode(ERequestMode::Owner);
    }

    return TMusicRequestBuilder(NApiPath::GenerativeFeedback(generativeStationId, streamId), meta, clientInfo,
                                logger, enableCrossDc, musicRequestModeInfoBuilder.BuildAndMove(), "GenerativeFedback")
        .SetMethod(NAppHostHttp::THttpRequest_EMethod_Post)
        .SetBody(payloadJsonStr)
        .SetUseOAuth()
        .Build(/* logVerbose = */ true);
}

NAppHostHttp::THttpRequest PrepareRadioFeedbackProxyRequestImpl(const TString& type, const TString& trackId, TInstant timestamp, float playedSec,
                                                       bool isIncognitoUser, const TString& batchId, const TString& radioSessionId,
                                                       TAtomicSharedPtr<IRequestMetaProvider> metaProvider, const TClientInfo& clientInfo,
                                                       const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo, TRTLogger& logger) {
    LOG_INFO(logger) << "Preparing radio feedback proxy request";

    NJson::TJsonValue payloadJson;
    payloadJson["incognito"] = isIncognitoUser;
    payloadJson["type"] = type;
    payloadJson["timestamp"] = timestamp.ToStringUpToSeconds();

    if (type == RADIO_FEEDBACK_TYPE_RADIO_STARTED) {
        // do nothing
    } else if (type == RADIO_FEEDBACK_TYPE_TRACK_STARTED || type == RADIO_FEEDBACK_TYPE_LIKE) {
        payloadJson["trackId"] = trackId; // Value is actually <trackId>:<albumId>
    } else if (type == RADIO_FEEDBACK_TYPE_SKIP || type == RADIO_FEEDBACK_TYPE_DISLIKE || type == RADIO_FEEDBACK_TYPE_TRACK_FINISHED) {
        payloadJson["trackId"] = trackId; // Value is actually <trackId>:<albumId>
        payloadJson["totalPlayedSeconds"] = playedSec;
    } else {
        ythrow yexception() << "Unsupported type of feedback, " << payloadJson;
    }

    // alice-tracks API doesn't have CGI, we pass them in body and path
    NJson::TJsonValue payloadWrapperJson;
    payloadWrapperJson["event"] = std::move(payloadJson);
    if (!batchId.Empty()) {
        payloadWrapperJson["batchId"] = batchId;
    }
    payloadJson = std::move(payloadWrapperJson);

    const TString path = NApiPath::RadioSessionFeedback(radioSessionId);
    return TMusicRequestBuilder(path, metaProvider, clientInfo, logger, enableCrossDc,
                                musicRequestModeInfo, "RadioFeedback")
        .SetMethod(NAppHostHttp::THttpRequest_EMethod_Post)
        .SetBody(JsonToString(payloadJson))
        .SetUseOAuth()
        .Build(/* logVerbose = */ true);
}

NAppHostHttp::THttpRequest PrepareRadioFeedbackProxyRequest(const NJson::TJsonValue& radioFeedbackJson, TInstant timestamp, float playedSec,
                                                   bool isIncognitoUser, const NScenarios::TRequestMeta& meta,
                                                   const TClientInfo& clientInfo, const bool enableCrossDc,
                                                   TRTLogger& logger) {
    const auto& type = ConvertRadioFeedbackType(radioFeedbackJson["type"].GetStringSafe());

    TString trackId = "";
    if (type == RADIO_FEEDBACK_TYPE_TRACK_STARTED || type == RADIO_FEEDBACK_TYPE_LIKE || type == RADIO_FEEDBACK_TYPE_SKIP ||
        type == RADIO_FEEDBACK_TYPE_DISLIKE || type == RADIO_FEEDBACK_TYPE_TRACK_FINISHED)
    {
        trackId = radioFeedbackJson["trackId"].GetString(); // Value is actually <trackId>:<albumId>
    }

    auto musicRequestModeInfoBuilder = TMusicRequestModeInfoBuilder().SetAuthMethod(EAuthMethod::OAuth);
    auto guestOAuthTokenEncrypted = radioFeedbackJson["guestOAuthTokenEncrypted"].GetStringSafe("");
    TAtomicSharedPtr<IRequestMetaProvider> metaProvider;
    if (!guestOAuthTokenEncrypted.Empty()) {
        metaProvider = MakeGuestRequestMetaProvider(meta, guestOAuthTokenEncrypted);
        musicRequestModeInfoBuilder.SetRequestMode(ERequestMode::ClientBiometry);
    } else {
        metaProvider = MakeAtomicShared<TRequestMetaProvider>(meta);
        if (isIncognitoUser) {
            musicRequestModeInfoBuilder.SetRequestMode(ERequestMode::Incognito);
        } else {
            musicRequestModeInfoBuilder.SetRequestMode(ERequestMode::Owner);
        }
    }

    // It's okay for some radio feedbacks to have empty batchId or radioSessionId
    // After https://st.yandex-team.ru/MUSICBACKEND-6494 all radioSessionIds will be non-empty
    const auto batchId = radioFeedbackJson["batchId"].GetStringSafe("");
    const auto radioSessionId = radioFeedbackJson["radioSessionId"].GetStringSafe(""); // TODO(sparkle): maybe ensure this isn't empty?

    return PrepareRadioFeedbackProxyRequestImpl(type, trackId, timestamp, playedSec, isIncognitoUser, batchId, radioSessionId, metaProvider,
                                                clientInfo, enableCrossDc, musicRequestModeInfoBuilder.BuildAndMove(), logger);
}

void ProcessEvent(TInstant timestamp, float playedSec, float offsetSec, float saveProgressOffsetSec, float durationSec,
    const NScenarios::TRequestMeta& meta, const TClientInfo& clientInfo, TRTLogger& logger,
    const NJson::TJsonValue& event, THttpProxyRequestItemPairs& result,
    THashSet<TString>& events, const TBiometryData& biometryData, const TStringBuf ownerUserId,
    const TExpFlags& flags)
{
    const bool enableCrossDc = flags.contains(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);
    if (event.Has(PLAY_AUDIO_EVENT)) {
        LOG_INFO(logger) << "Processing play audio event";
        const auto& playAudioJson = event[PLAY_AUDIO_EVENT];
        result.push_back({PreparePlayAudioProxyRequest(playAudioJson, timestamp, playedSec, offsetSec, durationSec,
                                                       meta, clientInfo, enableCrossDc, ownerUserId, GetRequestMode(biometryData), logger),
                          TString(MUSIC_PLAYS_REQUEST_ITEM)});

        Y_ENSURE(!events.contains(PLAY_AUDIO_EVENT));
        events.insert(TString(PLAY_AUDIO_EVENT));

        // Add additional request to save progress
        if (playAudioJson.Has("should_save_progress") && playAudioJson["should_save_progress"].GetBoolean()) {
            result.push_back({PrepareSaveProgressRequest(playAudioJson, timestamp, saveProgressOffsetSec, durationSec,
                                                         meta, clientInfo, enableCrossDc, ownerUserId, GetRequestMode(biometryData), logger),
                              TString(MUSIC_SAVE_PROGRESS_REQUEST_ITEM)});
        }
    } else if (event.Has(GENERATIVE_FEEDBACK_EVENT)) {
        LOG_INFO(logger) << "Processing generative feedback event";
        const auto& generativeFeedbackJson = event[GENERATIVE_FEEDBACK_EVENT];
        result.push_back({PrepareGenerativeFeedbackProxyRequest(generativeFeedbackJson, timestamp, meta, clientInfo, enableCrossDc, logger),
                          TString(MUSIC_GENERATIVE_FEEDBACK_REQUEST_ITEM)});
    } else if (event.Has(RADIO_FEEDBACK_EVENT)) {
        const auto& radioFeedbackJson = event[RADIO_FEEDBACK_EVENT];
        const auto& radioSessionId = radioFeedbackJson["radioSessionId"].GetStringSafe();
        if (radioSessionId != RADIO_FEEDBACK_PUMPKIN_RADIO_SESSION_ID) {
            const auto& eventType = radioFeedbackJson["type"].GetStringSafe();
            LOG_INFO(logger) << "Processing radio feedback event: " << eventType;
            if (eventType == "Skip") {
                result.push_back({PrepareRadioFeedbackProxyRequest(radioFeedbackJson, timestamp, playedSec, biometryData.IsIncognitoUser, meta, clientInfo, enableCrossDc, logger),
                                  TString(MUSIC_RADIO_FEEDBACK_SKIP_REQUEST_ITEM)});
            } 
            else if (eventType == "RadioStarted") {
                result.push_back({PrepareRadioFeedbackProxyRequest(radioFeedbackJson, timestamp, playedSec, biometryData.IsIncognitoUser, meta, clientInfo, enableCrossDc, logger),
                                  TString(MUSIC_RADIO_FEEDBACK_RADIO_STARTED_REQUEST_ITEM)});
            }
            else if (eventType == "Dislike") {
                result.push_back({PrepareRadioFeedbackProxyRequest(radioFeedbackJson, timestamp, playedSec, biometryData.IsIncognitoUser, meta, clientInfo, enableCrossDc, logger),
                                  TString(MUSIC_RADIO_FEEDBACK_DISLIKE_REQUEST_ITEM)});
            }
            else if (eventType == "TrackStarted"){
                result.push_back({PrepareRadioFeedbackProxyRequest(radioFeedbackJson, timestamp, playedSec, biometryData.IsIncognitoUser, meta, clientInfo, enableCrossDc, logger),
                                  TString(MUSIC_RADIO_FEEDBACK_TRACK_STARTED_REQUEST_ITEM)});
            }
            else if (eventType == "TrackFinished"){
                result.push_back({PrepareRadioFeedbackProxyRequest(radioFeedbackJson, timestamp, playedSec, biometryData.IsIncognitoUser, meta, clientInfo, enableCrossDc, logger),
                                  TString(MUSIC_RADIO_FEEDBACK_TRACK_FINISHED_REQUEST_ITEM)});
            }
            else {
                result.push_back({PrepareRadioFeedbackProxyRequest(radioFeedbackJson, timestamp, playedSec, biometryData.IsIncognitoUser, meta, clientInfo, enableCrossDc, logger),
                                  TString(MUSIC_RADIO_FEEDBACK_REQUEST_ITEM)});
            }

            Y_ENSURE(!events.contains(eventType), eventType);
            events.insert(eventType);
        } else {
            LOG_INFO(logger) << "Ignore pumpkin radio feedback event";
        }
    } else if (event.Has(SHOT_FEEDBACK_EVENT)) {
        LOG_INFO(logger) << "Processing shot feedback event";
        const auto& shotFeedbackJson = event[SHOT_FEEDBACK_EVENT];
        result.push_back({PrepareShotFeedbackProxyRequest(shotFeedbackJson, meta, clientInfo, enableCrossDc, ownerUserId, GetRequestMode(biometryData), logger),
                          TString(MUSIC_SHOTS_FEEDBACK_REQUEST_ITEM)});
        events.insert(SHOT_FEEDBACK_EVENT);
    } else {
        ythrow yexception() << "Unsupported callback event: " << JsonToString(event);
    }
}

void EnsureEvents(const THashSet<TString>& events) {
    Y_ENSURE(events.size() > 0 || events.size() <= 3, events.size());
    Y_ENSURE(events.contains(PLAY_AUDIO_EVENT) || events.contains(SHOT_FEEDBACK_EVENT));
    if (events.size() == 2) {
        Y_ENSURE(!events.contains("Skip"), "Events should contain some other radio feedback event");
    } else if (events.size() == 3) {
        Y_ENSURE(events.contains("Skip") || events.contains("Dislike") || events.contains("RadioStarted"));
        // Plus some other radio feedback event
    }
}

} // namespace

std::unique_ptr<NScenarios::TScenarioCommitResponse> MakeCommitSuccessResponse() {
    TCommitResponseBuilder builder;
    builder.SetSuccess();
    return std::move(builder).BuildResponse();
}

THttpProxyRequestItemPair MakeTimestampGenerativeFeedbackProxyRequest(const TMusicQueueWrapper& mq,
    TAtomicSharedPtr<IRequestMetaProvider> metaProvider, const TClientInfo& clientInfo, TRTLogger& logger,
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest, const TString& type,
    const TMusicRequestModeInfo& musicRequestModeInfo)
{
    const auto timestamp = TInstant::MilliSeconds(applyRequest.ServerTimeMs());

    NJson::TJsonValue payloadJson;
    payloadJson["type"] = type;
    payloadJson["timestamp"] = timestamp.ToStringUpToSeconds();

    const auto apiPath = NApiPath::GenerativeFeedback(mq.ContentId().GetId(), mq.CurrentItem().GetTrackId());
    const bool enableCrossDc = applyRequest.HasExpFlag(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);

    auto builder = TMusicRequestBuilder(apiPath, metaProvider, clientInfo, logger,
                                        enableCrossDc, musicRequestModeInfo,
                                        TString(MUSIC_GENERATIVE_FEEDBACK_REQUEST_ITEM))
        .SetMethod(NAppHostHttp::THttpRequest_EMethod_Post)
        .SetBody(JsonToString(payloadJson))
        .SetUseOAuth();

    return {std::move(builder).BuildAndMove(), TString(MUSIC_GENERATIVE_FEEDBACK_REQUEST_ITEM)};
}

NAppHostHttp::THttpRequest PrepareLikeDislikeRadioFeedbackProxyRequest(const TMusicQueueWrapper& mq, const NScenarios::TRequestMeta& meta,
                                                              const TClientInfo& clientInfo, TRTLogger& logger,
                                                              const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                                              const TScenarioState& scState, const bool isLike,
                                                              const bool enableCrossDc)
{
    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();
    const auto& biometryOpts = scState.GetQueue().GetPlaybackContext().GetBiometryOptions();
    
    auto musicRequestModeInfo = MakeMusicRequestModeInfo(EAuthMethod::OAuth, applyArgs.GetAccountStatus().GetUid(), scState);
    auto metaProvider = MakeRequestMetaProviderFromPlaybackBiometry(meta, biometryOpts);
        
    auto biometryData = ProcessBiometryOrFallback(logger, applyRequest, applyArgs.GetAccountStatus().GetUid());
    bool isIncognitoRequest = biometryData.IsIncognitoUser || (biometryOpts.GetUserId() != biometryData.UserId);

    const auto& currentItem = mq.CurrentItem();
    const float playedSec = applyRequest.Proto().GetBaseRequest().GetDeviceState().GetAudioPlayer().GetPlayedMs() / 1000.0f;

    return PrepareRadioFeedbackProxyRequestImpl(
        isLike ? RADIO_FEEDBACK_TYPE_LIKE : RADIO_FEEDBACK_TYPE_DISLIKE,
        TStringBuilder{} << currentItem.GetTrackId() << ':' << currentItem.GetTrackInfo().GetAlbumId(),
        TInstant::MilliSeconds(applyRequest.ServerTimeMs()),
        Max(playedSec, MIN_PLAY_TIME_SEC),
        isIncognitoRequest, mq.GetRadioBatchId(), mq.GetRadioSessionId(), metaProvider, clientInfo, enableCrossDc, musicRequestModeInfo, logger
    );
}

std::variant<THttpProxyRequestItemPairs, NScenarios::TScenarioCommitResponse>
MakeMusicReportRequest(const NScenarios::TRequestMeta& meta,
                       const TClientInfo& clientInfo, TRTLogger& logger,
                       const NHollywood::TScenarioApplyRequestWrapper& applyRequest) {
    const auto& callback = *applyRequest.Input().GetCallback();
    Y_ENSURE(callback.HasPayload());

    const auto& callbackPayload = callback.GetPayload();
    const auto& audioPlayer = applyRequest.Proto().GetBaseRequest().GetDeviceState().GetAudioPlayer();

    if (callback.GetName() == MUSIC_THIN_CLIENT_ON_FAILED && audioPlayer.GetOffsetMs() == 0) {
        LOG_INFO(logger) << "Audio track playback had not started, no need to send /plays report";
        return *MakeCommitSuccessResponse();
    }

    const auto timestamp = TInstant::MilliSeconds(applyRequest.ServerTimeMs());
    float playedSec = audioPlayer.GetPlayedMs() / 1000.0f;
    float offsetSec = audioPlayer.GetOffsetMs() / 1000.0f;
    float durationSec = audioPlayer.GetDurationMs() / 1000.0f;

    // requirement of music backend:
    if (callback.GetName() == MUSIC_THIN_CLIENT_ON_STARTED_CALLBACK) {
        // on start these two MUST be zero
        offsetSec = 0.0f;
        playedSec = 0.0f;
    } else {
        // otherwise MUST be non zero
        if (abs(offsetSec) < EPS) {
            offsetSec = MIN_PLAY_TIME_SEC;
        }
        if (abs(playedSec) < EPS) {
            playedSec = MIN_PLAY_TIME_SEC;
        }
    }

    if (callback.GetName() == MUSIC_THIN_CLIENT_ON_FINISHED_CALLBACK) {
        // When whole track was played, offsetSec must be the same as durationSec
        offsetSec = durationSec;
    }

    const auto callbackPayloadJson = JsonFromProto(callbackPayload);
    LOG_INFO(logger) << callback.GetName() << " callbackPayloadJson is " << JsonToString(callbackPayloadJson);

    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();
    const auto biometryData = ProcessBiometryOrFallback(logger, applyRequest, TStringBuf{applyArgs.GetAccountStatus().GetUid()});

    THttpProxyRequestItemPairs result;
    if (callbackPayloadJson.Has("events")) {
        LOG_INFO(logger) << "Preparing requests to plays/feedback http_proxies";
        THashSet<TString> events;
        for (const auto& event : callbackPayloadJson["events"].GetArraySafe()) {
            ProcessEvent(timestamp, playedSec, offsetSec, audioPlayer.GetOffsetMs() / 1000.0f, durationSec, meta, clientInfo, logger, event,
                result, events, biometryData, applyArgs.GetAccountStatus().GetUid(),
                applyRequest.ExpFlags());
        }
        EnsureEvents(events);
    } else {
        LOG_INFO(logger) << "Callback does not have any event";
    }
    return result;
}

} // namespace NAlice::NHollywood::NMusic
