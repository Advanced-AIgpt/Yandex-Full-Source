#include "shots.h"
#include "music_common.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/content_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/track_album_id.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>

namespace NAlice::NHollywood::NMusic {

TString GetPrevTrackId(const TMusicQueueWrapper& mq) {
    TString prevTrackId;
    if (mq.HasPreviousItem()) {
        const auto& prevItem = mq.PreviousItem();
        TTrackAlbumId prevTrackAlbumId{prevItem.GetTrackId(), prevItem.GetTrackInfo().GetAlbumId()};
        prevTrackId = prevTrackAlbumId.ToString();
    }
    return prevTrackId;
}

TString GetNextTrackId(const TMusicQueueWrapper& mq) {
    TString nextTrackId;
    if (mq.HasCurrentItem()) {
        const auto& curItem = mq.CurrentItem();
        TTrackAlbumId curTrackAlbumId{curItem.GetTrackId(), curItem.GetTrackInfo().GetAlbumId()};
        nextTrackId = curTrackAlbumId.ToString();
    }
    return nextTrackId;
}

void AddShotsRequest(TScenarioHandleContext& ctx, const TStringBuf userId, const TMusicQueueWrapper& mq,
                     const TMusicRequestModeInfo& musicRequestModeInfo) {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    TString prevTrackId = GetPrevTrackId(mq);
    TString nextTrackId = GetNextTrackId(mq);

    auto path = NApiPath::AfterTrack(userId, mq.MakeFrom(), ContentTypeToText(mq.ContentId().GetType()),
                                     mq.ContentId().GetId(), prevTrackId, nextTrackId);
    const bool enableCrossDc = request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);
    AddMusicProxyRequest(ctx,
                         TMusicRequestBuilder(std::move(path), ctx.RequestMeta,
                                              request.ClientInfo(),
                                              ctx.Ctx.Logger(), enableCrossDc, musicRequestModeInfo).BuildAndMove(),
                         MUSIC_SHOTS_HTTP_REQUEST_ITEM);
}

void ProcessShotsResponse(TRTLogger& logger, const TStringBuf jsonStr, TMusicQueueWrapper& mq) {
    auto json = JsonFromString(jsonStr);
    const auto* shots = json.GetValueByPath("result.shotEvent.shots");
    if (!shots) {
        mq.MarkBeforeTrackSlot(mq.CurrentItem().GetTrackId());
        LOG_INFO(logger) << "No shots in response";
        return;
    }
    TVector<std::pair<int, TExtraPlayable_TShot>> shotsArr;
    for (const auto& shotJson: shots->GetArray()) {
        if (shotJson["status"].GetString() != "ready") {
            continue;
        }
        auto& shot = shotsArr.emplace_back(shotJson["order"].GetInteger(), TExtraPlayable_TShot()).second;
        shot.SetId(shotJson["shotId"].GetString());
        LOG_INFO(logger) << "Found shotId = " << shot.GetId();
        const auto& shotData = shotJson["shotData"];
        shot.SetText(shotData["shotText"].GetString());
        shot.SetTitle(shotData["shotType"]["title"].GetString());
        shot.SetMdsUrl(shotData["mdsUrl"].GetString());
        shot.SetCoverUri(shotData["coverUri"].GetString());
        shot.SetEventId(json["result"]["shotEvent"]["eventId"].GetString());
        shot.SetContext(to_lower(TContentId_EContentType_Name(mq.ContentId().GetType())));
        shot.SetContextItem(mq.ContentId().GetId());
        shot.SetPrevTrackId(GetPrevTrackId(mq));
        shot.SetNextTrackId(GetNextTrackId(mq));
        shot.SetFrom(mq.MakeFrom());

    }
    if (shotsArr.empty()) {
        mq.MarkBeforeTrackSlot(mq.CurrentItem().GetTrackId());
        return;
    }
    Sort(shotsArr, [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    for (auto& [order, shot] : shotsArr) {
        Y_UNUSED(order);
        mq.AddShotBeforeTrack(mq.CurrentItem().GetTrackId(), std::move(shot));
    }
}

NAppHostHttp::THttpRequest PrepareShotFeedbackProxyRequest(NJson::TJsonValue shotFeedbackJson,
                                                  const NScenarios::TRequestMeta& meta,
                                                  const TClientInfo& clientInfo,
                                                  const bool enableCrossDc,
                                                  const TStringBuf ownerUserId,
                                                  ERequestMode requestMode,
                                                  TRTLogger& logger) {
    LOG_INFO(logger) << "Preparing shot feedback proxy request";
    auto uid = shotFeedbackJson["uid"].GetStringSafe();

    shotFeedbackJson.EraseValue("uid");
    shotFeedbackJson["type"] = to_lower(shotFeedbackJson["type"].GetStringSafe());

    NJson::TJsonValue payloadJson;
    payloadJson["items"].AppendValue(std::move(shotFeedbackJson));

    auto payloadJsonStr = JsonToString(payloadJson);

    auto musicRequestModeInfo = TMusicRequestModeInfoBuilder()
                            .SetAuthMethod(EAuthMethod::UserId)
                            .SetRequestMode(requestMode)
                            .SetOwnerUserId(ownerUserId)
                            .SetRequesterUserId(uid)
                            .BuildAndMove();

    LOG_DEBUG(logger) << payloadJson;
    return TMusicRequestBuilder(NApiPath::ShotsFeedback(uid), meta, clientInfo, logger, enableCrossDc, musicRequestModeInfo, "ShotsFeedback")
        .SetMethod(NAppHostHttp::THttpRequest_EMethod_Post)
        .SetBody(payloadJsonStr)
        .Build();
}

THttpProxyRequestItemPair MakeShotsLikeDislikeFeedbackProxyRequest(
    const TMusicQueueWrapper& mq, const NScenarios::TRequestMeta& meta, const TClientInfo& clientInfo,
    TRTLogger& logger, const TStringBuf userId, bool isLike, const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo)
{
    const TString prevTrackId = GetPrevTrackId(mq);
    const TString nextTrackId = GetNextTrackId(mq);

    const auto apiPath = NApiPath::ShotsLikeDislikeFeedback(
        userId, mq.GetShotBeforeCurrentItem()->GetId(),
        prevTrackId, nextTrackId, ContentTypeToText(mq.ContentId().GetType()),
        mq.ContentId().GetId(), mq.MakeFrom(), isLike);

    auto builder = TMusicRequestBuilder(apiPath, meta, clientInfo, logger, enableCrossDc, musicRequestModeInfo, TString(MUSIC_SHOTS_FEEDBACK_REQUEST_ITEM))
        .SetMethod(NAppHostHttp::THttpRequest_EMethod_Post);

    return {std::move(builder).BuildAndMove(), TString(MUSIC_SHOTS_FEEDBACK_REQUEST_ITEM)};
}

bool IsThinClientShotPlaying(const TScenarioBaseRequestWrapper& request) {
    const auto& deviceState = request.BaseRequestProto().GetDeviceState();
    if (!deviceState.HasAudioPlayer()) {
        return false;
    }
    const auto& audioPlayer = deviceState.GetAudioPlayer();
    // TODO(@amullanurov): изменить на streamType после 80-й прошивки
    const auto& deviceStateStreamTitle = audioPlayer.GetCurrentlyPlaying().GetTitle();
    return deviceStateStreamTitle == "Шот от Алисы";
}

} // namespace NAlice::NHollywood::NMusic
