#include "musical_clips_defs.h"
#include "musical_clips_prepare_handle.h"
#include "utils.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/hollywood/library/scenarios/video_musical_clips/nlg/register.h>
#include <alice/hollywood/library/scenarios/video_musical_clips/proto/musical_clips.pb.h>

#include <alice/library/experiments/flags.h>

#include <library/cpp/timezone_conversion/convert.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NMusicalClips {

using TMusicalClipsRequest = NAlice::NHollywood::NMusicalClips::TMusicalClipsRequest;

std::pair<NAppHostHttp::THttpRequest, TStringBuf> FeedbackRadioStartedPrepareProxyImpl(
        const NScenarios::TRequestMeta& meta,
        const TClientInfo& clientInfo,
        TRTLogger& logger,
        ui64 serverTimeMs,
        const bool enableCrossDc
    )
{
    LOG_INFO(logger) << "Preparing radioStarted feedback request...";
    auto path = NMusic::NApiPath::RadioFeedback(MUSICAL_CLIPS_MTV_STATION);

    auto body = NJson::TJsonMap{};
    body["type"] = "radioStarted";
    body["timestamp"] = TInstant::MilliSeconds(serverTimeMs).ToString();
    // NOTE: We do not need "from" and "dashboardId" fields in body right now..

    auto musicRequestModeInfo = NMusic::TMusicRequestModeInfoBuilder().SetAuthMethod(NMusic::EAuthMethod::OAuth).BuildAndMove();

    auto req = NMusic::TMusicRequestBuilder(path, meta, clientInfo, logger, enableCrossDc,
                                            musicRequestModeInfo, "FeedbackRadioStarted")
        .SetMethod(NAppHostHttp::THttpRequest::Post)
        .SetBody(JsonToString(body))
        .SetUseOAuth()
        .Build();
    return {std::move(req), MUSIC_RADIO_FEEDBACK_STARTED_REQUEST_ITEM};
}

std::pair<NAppHostHttp::THttpRequest, TStringBuf> FeedbackTrackStartedPrepareProxyImpl(
        const TStringBuf trackId,
        const NScenarios::TRequestMeta& meta,
        const TClientInfo& clientInfo,
        TRTLogger& logger,
        ui64 serverTimeMs,
        const bool enableCrossDc
    )
{
    LOG_INFO(logger) << "Preparing trackStarted feedback request...";
    auto path = NMusic::NApiPath::RadioFeedback(MUSICAL_CLIPS_MTV_STATION);

    auto body = NJson::TJsonMap{};
    body["type"] = "trackStarted";
    body["timestamp"] = TInstant::MilliSeconds(serverTimeMs).ToString();
    body["trackId"] = trackId;

    auto musicRequestModeInfo = NMusic::TMusicRequestModeInfoBuilder().SetAuthMethod(NMusic::EAuthMethod::OAuth).BuildAndMove();

    auto req = NMusic::TMusicRequestBuilder(path, meta, clientInfo, logger, enableCrossDc,
                                            musicRequestModeInfo, "FeedbackTrackStarted")
        .SetMethod(NAppHostHttp::THttpRequest::Post)
        .SetBody(JsonToString(body))
        .SetUseOAuth()
        .Build();
    return {std::move(req), MUSIC_RADIO_FEEDBACK_STARTED_REQUEST_ITEM};
}

std::pair<NAppHostHttp::THttpRequest, TStringBuf> FeedbackTrackFinishedPrepareProxyImpl(
        ui64 totalPlayedSeconds,
        const TStringBuf trackId,
        const NScenarios::TRequestMeta& meta,
        const TClientInfo& clientInfo,
        TRTLogger& logger,
        ui64 serverTimeMs,
        const bool enableCrossDc
    )
{
    LOG_INFO(logger) << "Preparing trackFinished feedback request...";
    auto path = NMusic::NApiPath::RadioFeedback(MUSICAL_CLIPS_MTV_STATION);

    auto body = NJson::TJsonMap{};
    body["type"] = "trackFinished";
    body["timestamp"] = TInstant::MilliSeconds(serverTimeMs).ToString();
    body["trackId"] = trackId;
    body["totalPlayedSeconds"] = totalPlayedSeconds;

    auto musicRequestModeInfo = NMusic::TMusicRequestModeInfoBuilder().SetAuthMethod(NMusic::EAuthMethod::OAuth).BuildAndMove();

    LOG_INFO(logger) << "Track finished feedback body: " << JsonToString(body);
    auto req = NMusic::TMusicRequestBuilder(path, meta, clientInfo, logger, enableCrossDc,
                                            musicRequestModeInfo, "FeedbackTrackFinished")
        .SetMethod(NAppHostHttp::THttpRequest::Post)
        .SetBody(JsonToString(body))
        .SetUseOAuth()
        .Build();
    return {std::move(req), MUSIC_RADIO_FEEDBACK_REQUEST_ITEM};
}

std::pair<NAppHostHttp::THttpRequest, TStringBuf> FeedbackSkipPrepareProxyImpl(
        ui64 totalPlayedSeconds,
        const TStringBuf trackId,
        const NScenarios::TRequestMeta& meta,
        const TClientInfo& clientInfo,
        TRTLogger& logger,
        ui64 serverTimeMs,
        const bool enableCrossDc
    )
{
    LOG_INFO(logger) << "Preparing skip feedback request...";
    auto path = NMusic::NApiPath::RadioFeedback(MUSICAL_CLIPS_MTV_STATION);

    auto body = NJson::TJsonMap{};
    body["type"] = "skip";
    body["timestamp"] = TInstant::MilliSeconds(serverTimeMs).ToString();
    body["trackId"] = trackId;
    body["totalPlayedSeconds"] = totalPlayedSeconds;

    auto musicRequestModeInfo = NMusic::TMusicRequestModeInfoBuilder().SetAuthMethod(NMusic::EAuthMethod::OAuth).BuildAndMove();

    auto req = NMusic::TMusicRequestBuilder(path, meta, clientInfo, logger, enableCrossDc,
                                            musicRequestModeInfo, "FeedbackTrackSkiped")
        .SetMethod(NAppHostHttp::THttpRequest::Post)
        .SetBody(JsonToString(body))
        .SetUseOAuth()
        .Build();
    return {std::move(req), MUSIC_RADIO_FEEDBACK_REQUEST_ITEM};
}

std::pair<NAppHostHttp::THttpRequest, TStringBuf> RadioTracksPrepareProxyImpl(
        const TVector<TStringBuf>& queue,
        const NScenarios::TRequestMeta& meta,
        const TClientInfo& clientInfo,
        TRTLogger& logger,
        bool newRadioSession,
        const bool enableCrossDc
    )
{
    LOG_INFO(logger) << "Preparing RadioTracks feedback request...";
    auto path = NMusic::NApiPath::RadioTracks(MUSICAL_CLIPS_MTV_STATION, queue, newRadioSession);

    auto musicRequestModeInfo = NMusic::TMusicRequestModeInfoBuilder().SetAuthMethod(NMusic::EAuthMethod::OAuth).BuildAndMove();

    auto req = NMusic::TMusicRequestBuilder(path, meta, clientInfo, logger, enableCrossDc,
                                            musicRequestModeInfo, "StationTracks")
        .SetMethod(NAppHostHttp::THttpRequest::Get)
        .SetUseOAuth()
        .Build();
    return {std::move(req), MUSIC_RADIO_TRACKS_REQUEST_ITEM};
}

void TMusicalClipsPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};
    const auto serverTimeMs = requestProto.GetBaseRequest().GetServerTimeMs();

    const TMusicalClipsRequest applyArgs = request.UnpackArguments<TMusicalClipsRequest>();
    const auto continueType = TMusicalClipsRequest_ERequestContinueType_Name(applyArgs.GetType());
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    const bool enableCrossDc = request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);
    TContinueResponseBuilder builder(&nlgWrapper);
    if (continueType == "START") {
        LOG_INFO(ctx.Ctx.Logger()) << "Continue stage proceeds to START flow...";

        // Make feedback radio started request
        auto[req_feedback, item_feedback] = FeedbackRadioStartedPrepareProxyImpl(
            ctx.RequestMeta,
            request.ClientInfo(),
            ctx.Ctx.Logger(),
            serverTimeMs,
            enableCrossDc
        );
        NMusic::AddMusicProxyRequest(ctx, req_feedback, item_feedback);

        // Make station tracks request without queue
        TVector<TStringBuf> queue;
        auto[req_tracks, item_tracks] = RadioTracksPrepareProxyImpl(
            queue,
            ctx.RequestMeta,
            request.ClientInfo(),
            ctx.Ctx.Logger(),
            true,
            enableCrossDc
        );
        NMusic::AddMusicProxyRequest(ctx, req_tracks, item_tracks);

    } else  {
        LOG_INFO(ctx.Ctx.Logger()) << "Continue stage proceeds to second call...";
        ui64 endTimeMs = applyArgs.GetEndTimeMs();;
        TString trackId = applyArgs.GetMusicalTrackId();
        ui64 startTimeMs = applyArgs.GetStartTimeMs();
        ui64 totalPlayedSeconds = applyArgs.GetTotalPlayedSeconds();
        LOG_INFO(ctx.Ctx.Logger()) << "Radio feedback params: startTimeMs " << startTimeMs << ", endTimeMs " << endTimeMs << ", totalPlayedSeconds " << totalPlayedSeconds << ", trackId " << trackId;

        // Make feedback track started request
        auto[req_started, item_started] = FeedbackTrackStartedPrepareProxyImpl(
            trackId,
            ctx.RequestMeta,
            request.ClientInfo(),
            ctx.Ctx.Logger(),
            startTimeMs,
            enableCrossDc
        );
        NMusic::AddMusicProxyRequest(ctx, req_started, item_started);

        // Make station tracks request with queue
        TVector<TStringBuf> queue;
        queue.push_back(trackId);
        auto[req_tracks, item_tracks] = RadioTracksPrepareProxyImpl(
            queue,
            ctx.RequestMeta,
            request.ClientInfo(),
            ctx.Ctx.Logger(),
            false,
            enableCrossDc
        );
        NMusic::AddMusicProxyRequest(ctx, req_tracks, item_tracks);

        if (continueType == "NEXT") {
            LOG_INFO(ctx.Ctx.Logger()) << "Continue NEXT stage due alice.quasar.video_player.finished semantic frame."
            << " trackId " << trackId;

            // Make feedback track finished request
            auto[req_feedback, item_feedback] = FeedbackTrackFinishedPrepareProxyImpl(
                totalPlayedSeconds,
                trackId,
                ctx.RequestMeta,
                request.ClientInfo(),
                ctx.Ctx.Logger(),
                endTimeMs,
                enableCrossDc
            );
            NMusic::AddMusicProxyRequest(ctx, req_feedback, item_feedback);

        } else if (continueType == "SKIP") {
            LOG_INFO(ctx.Ctx.Logger()) << "Continue SKIP stage due personal_assistant.scenarios.player.next_track semantic frame.";


            // Make feedback track finished request
            auto[req_feedback, item_feedback] = FeedbackSkipPrepareProxyImpl(
                totalPlayedSeconds,
                trackId,
                ctx.RequestMeta,
                request.ClientInfo(),
                ctx.Ctx.Logger(),
                endTimeMs,
                enableCrossDc
            );
            NMusic::AddMusicProxyRequest(ctx, req_feedback, item_feedback);

        } else {
            LOG_INFO(ctx.Ctx.Logger()) << "Continue stage have got irrelevant frame.";
            AddIrrelevantResponse(ctx);
            return;
        }
    }

    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}
} // namespace NAlice::NHollywood::NMusicalClips
