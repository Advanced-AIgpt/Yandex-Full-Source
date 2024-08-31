#include "play_video.h"
#include "helper.h"

#include <alice/hollywood/library/framework/core/codegen/gen_directives.pb.h>
#include <alice/hollywood/library/scenarios/video/analytics.h>
#include <alice/hollywood/library/scenarios/video/video_utils.h>

#include <alice/library/video_common/device_helpers.h>
#include <alice/library/video_common/frontend_vh_helpers/frontend_vh_requests.h>


using namespace NAlice::NHollywood::NVideo;
namespace NAlice::NHollywoodFw::NVideo {

inline constexpr TStringBuf FRONTEND_VH_PLAYER_REQUEST_ITEM = "hw_frontend_vh_player_request";
inline constexpr TStringBuf FRONTEND_VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM = "hw_frontend_vh_player_request_rtlog_token";

inline constexpr TStringBuf FRONTEND_VH_SEASONS_REQUEST_ITEM = "hw_frontend_vh_seasons_request";
inline constexpr TStringBuf FRONTEND_VH_SEASONS_REQUEST_RTLOG_TOKEN_ITEM = "hw_frontend_vh_seasons_request_rtlog_token";

inline constexpr TStringBuf PIRATE_VIDEO = "pirate_video";


NScenarios::TVideoPlayDirective MakeVideoPlayDirective(const TVideoPlaySceneArgs& sceneArgs) {
    NScenarios::TVideoPlayDirective directive;

    const auto& vhPlayerData = sceneArgs.GetAdditionalVhPlayerData();

    directive.SetName(sceneArgs.GetVideoItem().GetName());
    directive.SetUri(sceneArgs.GetVideoItem().GetPlayUri());
    directive.SetPayload(vhPlayerData.GetPayload());
    directive.SetSessionToken(vhPlayerData.GetSessionToken());
    *directive.MutableItem() = sceneArgs.GetVideoItem();
    if (sceneArgs.HasTvShowItem()) {
        *directive.MutableTvShowItem() = sceneArgs.GetTvShowItem();
    }
    directive.SetStartAt(vhPlayerData.GetStartAt());
    *directive.MutableAudioLanguage() = vhPlayerData.GetAudioLanguage();
    *directive.MutableSubtitlesLanguage() = vhPlayerData.GetSubtitlesLanguage();

    return directive;
}

TRetSetup TVideoPlayScene::MainSetup(const TVideoVhArgs& args, const TRunRequest& request, const TStorage&) const {
    TSetup setup(request);
    if (!args.GetProviderItemId()) {
        LOG_WARN(request.Debug().Logger()) << "No providerItemId for item!";

        auto deviceState = request.Client().TryGetMessage<TDeviceState>();
        Y_ENSURE(deviceState, "Unable to get TDeviceState");
        if (NVideoCommon::IsContentDetailsScreen(*deviceState)) {
            LOG_WARN(request.Debug().Logger()) << "Pirate video on content details screen detected";
            setup.AttachRequest(PIRATE_VIDEO, GetVideoItemFromContentDetailsScreen(*deviceState));
        } else {
            TError err(TError::EErrorDefinition::Unknown);
            err.Details() << "Could not setup video play scene";
            return err;
        }
    } else {
        LOG_INFO(request.Debug().Logger()) << "Successfully attached vh request";
        setup.AttachRequest(VH_PROXY_REQUEST, args);
    }
    return setup;
}

TRetMain TVideoPlayScene::Main(const TVideoVhArgs&,
                               const TRunRequest& request,
                               TStorage&,
                               const TSource& source) const {
    auto& logger = request.Debug().Logger();

    if (TVideoItem item; source.GetSource(PIRATE_VIDEO, item)) {
        NVideoCommon::TVideoFeatures vf;
        vf.SetIsSearchVideo(true);
        TRunFeatures features = FillIntentData(request, false, vf);
        LOG_INFO(logger) << "Gor pirate videoItem in Main";
        return TReturnValueRender(&TVideoPlayScene::PirateVideoOpen, item, std::move(features));
    }

    TVideoPlaySceneArgs args;
    if (!source.GetSource(VH_PROXY_RESPONSE, args)) {
        TError err(TError::EErrorDefinition::ExternalError);
        err.Details() << "No vh response found";
        return err;
    }

    switch (args.GetResultCase()) {
        case TVideoPlaySceneArgs::kAdditionalVhPlayerData:
            if (args.GetAdditionalVhPlayerData().GetHasActiveLicenses()) {
                LOG_INFO(logger) << "Video " << args.GetVideoItem().GetProviderItemId() << " has active licenses. Rendering play!";
                TRunFeatures features = FillIntentData(request, true);
                return TReturnValueRender(&TVideoPlayScene::PlayRender, args, std::move(features));
            } else {
                LOG_INFO(logger) << "Video " << args.GetVideoItem().GetProviderItemId() << " has NO licenses. ";
                NVideoCommon::TVideoFeatures vf;
                TVideoDetailsScreenArgs detailsScreenArgs;
                *detailsScreenArgs.MutableVideoItem() = std::move(args.HasTvShowItem() ? args.GetTvShowItem() : args.GetVideoItem());
                if (NVideoCommon::IsContentDetailsScreen(request.GetRunRequest().GetBaseRequest().GetDeviceState())) {
                    LOG_INFO(logger) << "Currently on content details screen. Paid warning instead of play";
                    detailsScreenArgs.SetOnlyWarn(1);
                } else {
                    vf.SetIsGoToVideoScreen(1);
                    LOG_INFO(logger) << "Opening content details with can't play paid video warn";
                }
                TRunFeatures features = FillIntentData(request, false, vf);
                return TReturnValueRender(&TVideoPlayScene::DetailsScreenOpen, detailsScreenArgs, std::move(features));
            }
        case TVideoPlaySceneArgs::kNoSuchSeason:
            {
                TVideoDetailsScreenArgs detailsScreenArgs;
                NVideoCommon::TVideoFeatures vf;
                vf.SetIsOpenCurrentVideo(1);
                vf.SetNoSuchSeason(1);
                // for details screen if a tv_show provided, only TvShowItem could be used.
                *detailsScreenArgs.MutableVideoItem() = std::move(args.HasTvShowItem() ? args.GetTvShowItem() : args.GetVideoItem());
                detailsScreenArgs.SetWarnNoSuchSeason(1);
                TRunFeatures features = FillIntentData(request, false, vf);
                return TReturnValueRender(&TVideoPlayScene::DetailsScreenOpen, detailsScreenArgs, std::move(features));
            }
        case TVideoPlaySceneArgs::kNoSuchEpisode:
            {
                TVideoDetailsScreenArgs detailsScreenArgs;
                NVideoCommon::TVideoFeatures vf;
                vf.SetIsOpenCurrentVideo(1);
                vf.SetNoSuchEpisode(1);
                *detailsScreenArgs.MutableVideoItem() = std::move(args.HasTvShowItem() ? args.GetTvShowItem() : args.GetVideoItem());
                detailsScreenArgs.SetWarnNoSuchEpisode(1);
                TRunFeatures features = FillIntentData(request, false, vf);
                return TReturnValueRender(&TVideoPlayScene::DetailsScreenOpen, detailsScreenArgs, std::move(features));
            }
        case TVideoPlaySceneArgs::RESULT_NOT_SET:
            TError err(TError::EErrorDefinition::SubsystemError);
            err.Details() << "Undefined VH response. Args: " << args.ShortUtf8DebugString();
            return err;
    }
}

TRetResponse TVideoPlayScene::PlayRender(const TVideoPlaySceneArgs &args, TRender &render) {
    LOG_INFO(render.GetRequest().Debug().Logger()) << "Rendering video play scene with args: " << args.Utf8DebugString();
    render.Directives().AddVideoPlayDirective(MakeVideoPlayDirective(args));
    render.CreateFromNlg("video", "video_autoplay", NJson::TJsonValue{});
    *render.GetResponseBody().MutableAnalyticsInfo()->AddObjects() = GetAnalyticsObjectCurrentlyPlayingVideo(args.GetVideoItem());
    return TReturnValueSuccess();
}

TRetResponse TVideoPlayScene::DetailsScreenOpen(const TVideoDetailsScreenArgs &args, TRender &render) {
    if (!args.HasOnlyWarn()) {
        render.Directives().AddTvOpenDetailsScreenDirective(NHollywood::NVideo::MakeTvOpenDetailsScreenDirective(args.GetVideoItem()));
        *render.GetResponseBody().MutableAnalyticsInfo()->AddObjects() = NAlice::NHollywood::NVideo::GetAnalyticsObjectForDescription(args.GetVideoItem(), false);
    }

    if (args.HasWarnNoSuchSeason()) {
        LOG_INFO(render.GetRequest().Debug().Logger()) << "Rendering no such season warn";
        render.CreateFromNlg("video", "no_such_season", NJson::TJsonValue{});
    } else if (args.HasWarnNoSuchEpisode()) {
        LOG_INFO(render.GetRequest().Debug().Logger()) << "Rendering no such episode warn";
        render.CreateFromNlg("video", "no_such_episode", NJson::TJsonValue{});
    } else if (args.HasOnlyWarn()) {
        LOG_INFO(render.GetRequest().Debug().Logger()) << "Rendering only warn about paid video";
        render.CreateFromNlg("video", "video_tv_payment_without_push", NJson::TJsonValue{});
    } else {
        LOG_INFO(render.GetRequest().Debug().Logger()) << "Rendering open details screen scene with args: " << args.GetVideoItem().Utf8DebugString();
        render.CreateFromNlg("video", "cannot_autoplay_paid_video", NJson::TJsonValue{});
    }
    return TReturnValueSuccess();
}

TRetResponse TVideoPlayScene::PirateVideoOpen(const TVideoItem &item, TRender &render) {
    LOG_INFO(render.GetRequest().Debug().Logger()) << "Rendering open search by pirate video name";
    render.Directives().AddTvOpenSearchScreenDirective(MakeTvOpenSearchScreenDirective(item));
    render.CreateFromNlg("video", "no_relevant_video", NJson::TJsonValue{});
    return TReturnValueSuccess();
}

} // namespace NAlice::NHollywoodFw::NVideo
