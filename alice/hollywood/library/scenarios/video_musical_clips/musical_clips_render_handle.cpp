#include "musical_clips_render_handle.h"
#include "utils.h"
#include "musical_clips_defs.h"

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/hollywood/library/scenarios/video_musical_clips/proto/musical_clips.pb.h>
#include <alice/library/video_common/frontend_vh_helpers/video_item_helper.h>
#include <alice/library/video_common/hollywood_helpers/util.h>
#include <alice/library/video_common/vh_player.h>

namespace NAlice::NHollywood::NMusicalClips {

bool IsSilentResponse(const TScenarioApplyRequestWrapper& request) {
    if (const auto semanticFrame = request.Input().FindSemanticFrame(ALICE_SHOW_MUSICAL_CLIPS)) {
        return false;
    }

    return true;
}

void PlayVhMusicalClip(
        const TString& trackId,
        TScenarioHandleContext& ctx,
        const TScenarioApplyRequestWrapper& applyRequest,
        TResponseBodyBuilder& bodyBuilder,
        const NVideoCommon::TVideoItemHelper& videoItemHelper
    ) {

    NScenarios::TDirective oneOfDirective;

    NScenarios::TScenarioRunRequest requestRunProto;
    auto baseRequest = applyRequest.Proto().GetBaseRequest();
    *requestRunProto.MutableBaseRequest() = std::move(baseRequest);
    auto input = applyRequest.Proto().GetInput();
    *requestRunProto.MutableInput() = std::move(input);
    const TScenarioRunRequestWrapper request{requestRunProto, ctx.ServiceCtx};

    *oneOfDirective.MutableVideoPlayDirective() = videoItemHelper.MakeVideoPlayDirective(request);
    NScenarios::TVideoPlayDirective& vhDirective = *oneOfDirective.MutableVideoPlayDirective();
    TVideoItem& item = *vhDirective.MutableItem();

    const TMusicalClipsRequest applyArgs = applyRequest.UnpackArguments<TMusicalClipsRequest>();
    const auto continueType = NAlice::NHollywood::NMusicalClips::TMusicalClipsRequest_ERequestContinueType_Name(applyArgs.GetType());
    // Skip yandex.music intro for nonfirst clips
    ui64 startTime = 3;
    if (continueType == "START") {
        startTime = 0;
    }

    item.SetMusicalTrackId(trackId);

    vhDirective.SetStartAt(startTime);

    bodyBuilder.AddDirective(std::move(oneOfDirective));
    TNlgData nlgData(ctx.Ctx.Logger(), request);
    nlgData.AddAttention(NAlice::NVideoCommon::ATTENTION_AUTOPLAY);
    if (!IsSilentResponse(applyRequest)) {
        bodyBuilder.AddRenderedTextWithButtonsAndVoice("video_musical_clips", "render_result", /* buttons = */ {}, nlgData);
    }
}

void PlayYoutubeMusicalClip(
        const TString& trackId,
        TScenarioHandleContext& ctx,
        const TScenarioApplyRequestWrapper& request,
        TResponseBodyBuilder& bodyBuilder,
        const NJson::TJsonValue& youtubeClip,
        ui64 duration
    ) {
    const auto play_uri = TStringBuilder{} << "youtube://" << youtubeClip["providerVideoId"].GetStringRobust();
    LOG_INFO(ctx.Ctx.Logger()) << "TMusicalClipsRenderHandle play youtube: " << play_uri;
    NScenarios::TDirective oneOfDirective;

    NScenarios::TVideoPlayDirective youtubeDirective;
    youtubeDirective.SetName("video_play");
    youtubeDirective.SetUri(play_uri);
    youtubeDirective.SetStartAt(0);

    TVideoItem item;
    const TMusicalClipsRequest applyArgs = request.UnpackArguments<TMusicalClipsRequest>();
    const auto continueType = NAlice::NHollywood::NMusicalClips::TMusicalClipsRequest_ERequestContinueType_Name(applyArgs.GetType());

    item.SetMusicalTrackId(trackId);
    item.SetType("video");
    item.SetProviderName("youtube");
    item.SetProviderItemId(youtubeClip["providerVideoId"].GetStringRobust());
    item.SetAvailable(1);
    item.SetName(youtubeClip["title"].GetStringRobust());
    item.SetDuration(duration);
    item.SetPlayUri(play_uri);
    item.SetSourceHost("www.youtube.com");
    item.SetThumbnailUrl16x9("https://yastatic.net/s3/home/station/mordovia/doc2doc_video/blank.png ");
    item.SetThumbnailUrl16x9Small("https://yastatic.net/s3/home/station/mordovia/doc2doc_video/blank.png ");

    *youtubeDirective.MutableItem() = std::move(item);
    *oneOfDirective.MutableVideoPlayDirective() = std::move(youtubeDirective);
    bodyBuilder.AddDirective(std::move(oneOfDirective));
    TNlgData nlgData(ctx.Ctx.Logger(), request);
    nlgData.AddAttention(NAlice::NVideoCommon::ATTENTION_AUTOPLAY);
    if (!IsSilentResponse(request)) {
        bodyBuilder.AddRenderedTextWithButtonsAndVoice("video_musical_clips", "render_result", /* buttons = */ {}, nlgData);
    }
}

TStringBuf GetIntentName (const TString continueType) {
    if (continueType == "START") {
        return ALICE_SHOW_MUSICAL_CLIPS;
    } else if (continueType == "NEXT") {
        return ALICE_PLAYER_FINISHED;
    } else if (continueType == "SKIP") {
        return ALICE_PLAYER_NEXT_TRACK;
    } else if (continueType == "PREVIOUS") {
        return ALICE_PLAYER_PREV_TRACK;
    } else if (continueType == "REPLAY") {
        return ALICE_PLAYER_REPLAY;
    } else if (continueType == "LIKE") {
        return ALICE_PLAYER_LIKE;
    } else if (continueType == "DISLIKE") {
        return ALICE_PLAYER_DISLIKE;
    }

    return "unknown intent";
}

void TMusicalClipsRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TApplyResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    const TMusicalClipsRequest applyArgs = request.UnpackArguments<TMusicalClipsRequest>();
    NVideoCommon::FillAnalyticsInfo(
            bodyBuilder,
            GetIntentName(TMusicalClipsRequest_ERequestContinueType_Name(applyArgs.GetType())),
            ANALYTICS_PRODUCT_SCENARIO_NAME
        );

    const auto stationTracksResponse = RetireHttpResponseJson(
        ctx,
        MUSIC_RADIO_TRACKS_RESPONSE_ITEM,
        NMusic::MUSIC_REQUEST_RTLOG_TOKEN_ITEM
    );

    for (int trackNumber = 0; trackNumber < 5; trackNumber++) {
        TString responseVhName = MakeNumberedName(FRONTEND_VH_PLAYER_RESPONSE_ITEM, trackNumber);
        const TMaybe<NJson::TJsonValue> vhResponse = RetireHttpResponseJsonMaybe(
            ctx,
            responseVhName,
            FRONTEND_VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM
        );
        if (!vhResponse.Defined()) {
            LOG_INFO(ctx.Ctx.Logger()) << responseVhName << " not defined.";
            continue;
        }

        auto videoItemHelper = NVideoCommon::TVideoItemHelper::TryMakeFromVhPlayerResponse(*vhResponse);
        if (!videoItemHelper.Defined())
            continue;

        // Get trackId
        TString trackId = (stationTracksResponse["result"]["sequence"].GetArray())[trackNumber]["track"]["id"].GetStringRobust();
        TString albumId = (stationTracksResponse["result"]["sequence"].GetArray())[trackNumber]["track"]["albums"][0]["id"].GetStringRobust();
        TString completeTrackId = TStringBuilder() << trackId << ":" << albumId;
        PlayVhMusicalClip(completeTrackId, ctx, request, bodyBuilder, *videoItemHelper);

        auto response = std::move(builder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        return;
    }

    for (int trackNumber = 0; trackNumber < 5; trackNumber++) {
        TString responseName = MakeNumberedName(MUSIC_CLIP_RESPONSE_ITEM, trackNumber);
        const TMaybe<NJson::TJsonValue> searchClipsResponse = RetireHttpResponseJsonMaybe(
                    ctx,
                    responseName,
                    NMusic::MUSIC_REQUEST_RTLOG_TOKEN_ITEM
                );

        if (!searchClipsResponse.Defined()) {
            continue;
        }

        for (const auto& clip: (*searchClipsResponse)["result"]["videos"].GetArray()) {
            TString provider = clip["provider"].GetStringRobust();
            if (provider != "youtube")
                continue;

            // Get trackId
            TString trackId = (stationTracksResponse["result"]["sequence"].GetArray())[trackNumber]["track"]["id"].GetStringRobust();
            TString albumId = (stationTracksResponse["result"]["sequence"].GetArray())[trackNumber]["track"]["albums"][0]["id"].GetStringRobust();
            TString completeTrackId = TStringBuilder() << trackId << ":" << albumId;
            //TODO: get real video duration from supplement handle
            ui64 duration = (stationTracksResponse["result"]["sequence"].GetArray())[trackNumber]["track"]["durationMs"].GetIntegerSafe() / 1000u;
            PlayYoutubeMusicalClip(completeTrackId, ctx, request, bodyBuilder, clip, duration);

            auto response = std::move(builder).BuildResponse();
            ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
            return;
        }
    }

    LOG_INFO(ctx.Ctx.Logger()) << "There are not vh or youtube clips in the queue.";
    AddIrrelevantResponse(ctx);
    return;
}

} // namespace NAlice::NHollywood::NMusicalClips
