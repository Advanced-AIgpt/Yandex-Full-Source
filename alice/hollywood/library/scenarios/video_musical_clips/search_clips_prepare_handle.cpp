#include "search_clips_prepare_handle.h"
#include "musical_clips_defs.h"
#include "utils.h"

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/hollywood/library/scenarios/video_musical_clips/proto/musical_clips.pb.h>

#include <alice/library/experiments/flags.h>

namespace NAlice::NHollywood::NMusicalClips {

std::pair<NAppHostHttp::THttpRequest, TStringBuf> SearchClipPrepareProxyImpl(
    const TStringBuf trackId,
    const NScenarios::TRequestMeta& meta,
    const TClientInfo& clientInfo,
    TRTLogger& logger,
    int trackNumber,
    const bool enableCrossDc)
{
    LOG_INFO(logger) << "Preparing Supplement, Track Id: " << trackId;
    const auto path = TStringBuilder{} << "/tracks/" << trackId << "/supplement";
    TString requestName = MakeNumberedName(MUSIC_CLIP_REQUEST_ITEM, trackNumber);
    LOG_INFO(logger) << "requestName " << requestName;

    auto musicRequestModeInfo = NMusic::TMusicRequestModeInfoBuilder().SetAuthMethod(NMusic::EAuthMethod::OAuth).BuildAndMove();

    auto req = NMusic::TMusicRequestBuilder(path, meta, clientInfo, logger, enableCrossDc,
                                            musicRequestModeInfo, "ClipSearchStarted")
        .SetMethod(NAppHostHttp::THttpRequest::Get)
        .SetUseOAuth()
        .Build();
    return {std::move(req), requestName};
}

void TSearchClipsPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};
    const TMusicalClipsRequest applyArgs = request.UnpackArguments<TMusicalClipsRequest>();
    const auto continueType = TMusicalClipsRequest_ERequestContinueType_Name(applyArgs.GetType());
    const bool enableCrossDc = request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);

    if (EqualToOneOf(continueType, "START", "NEXT", "SKIP")) {
        const auto feedbackStartedResponse = RetireHttpResponseJsonMaybe(
                                ctx,
                                MUSIC_RADIO_FEEDBACK_STARTED_RESPONSE_ITEM, NMusic::MUSIC_REQUEST_RTLOG_TOKEN_ITEM
                            );
        if (!feedbackStartedResponse.Defined() || (*feedbackStartedResponse)["result"] != "ok") {
            if (continueType == "START") {
                LOG_INFO(ctx.Ctx.Logger()) << "Radio started feedback result is not OK";
            } else {
                LOG_INFO(ctx.Ctx.Logger()) << "Track started feedback result is not OK";
            }
        }

        if (continueType != "START") {
            const auto feedbackResponse = RetireHttpResponseJsonMaybe(
                                    ctx,
                                    MUSIC_RADIO_FEEDBACK_RESPONSE_ITEM, NMusic::MUSIC_REQUEST_RTLOG_TOKEN_ITEM
                                );
            if (!feedbackResponse.Defined() || (*feedbackResponse)["result"] != "ok") {
                LOG_INFO(ctx.Ctx.Logger()) << "Track finished feedback result is not OK";
            }
        }
    }

    const auto stationTracksResponse = RetireHttpResponseJsonMaybe(
                            ctx,
                            MUSIC_RADIO_TRACKS_RESPONSE_ITEM, NMusic::MUSIC_REQUEST_RTLOG_TOKEN_ITEM
                        );
    if (stationTracksResponse.Defined()) {
        int trackNumber = 0;
        for (const auto& track: (*stationTracksResponse)["result"]["sequence"].GetArray()) {
            if (trackNumber > 4)
                break;
            auto[req, item] = SearchClipPrepareProxyImpl(
                track["track"]["id"].GetStringRobust(),
                ctx.RequestMeta,
                request.ClientInfo(),
                ctx.Ctx.Logger(),
                trackNumber,
                enableCrossDc
            );

            NMusic::AddMusicProxyRequest(ctx, req, item);
            trackNumber ++;
        }
    }
}

} // namespace NAlice::NHollywood::NMusicalClips
