#include "video_vh.h"
#include "video_utils.h"

#include <alice/hollywood/library/environment_state/environment_state.h>
#include <alice/library/video_common/frontend_vh_helpers/video_item_helper.h>
#include <alice/library/video_common/frontend_vh_helpers/frontend_vh_requests.h>

#include <google/protobuf/wrappers.pb.h>


using namespace NAlice::NHollywoodFw::NVideo;
namespace NAlice::NHollywood::NVideo {

inline constexpr TStringBuf FRONTEND_VH_GET_LAST_REQUEST_ITEM = "hw_frontend_vh_get_last_request";
inline constexpr TStringBuf FRONTEND_VH_GET_LAST_RESPONSE_ITEM = "hw_frontend_vh_get_last_response";
inline constexpr TStringBuf FRONTEND_VH_GET_LAST_REQUEST_RTLOG_TOKEN_ITEM = "hw_frontend_vh_get_last_request_rtlog_token";
inline constexpr TStringBuf FRONTEND_VH_SEASONS_REQUEST_ITEM = "hw_frontend_vh_seasons_request";
inline constexpr TStringBuf FRONTEND_VH_SEASONS_RESPONSE_ITEM = "hw_frontend_vh_seasons_response";
inline constexpr TStringBuf FRONTEND_VH_SEASONS_REQUEST_RTLOG_TOKEN_ITEM = "hw_frontend_vh_seasons_request_rtlog_token";
inline constexpr TStringBuf FRONTEND_VH_EPISODES_REQUEST_ITEM = "hw_frontend_vh_episodes_request";
inline constexpr TStringBuf FRONTEND_VH_EPISODES_RESPONSE_ITEM = "hw_frontend_vh_episodes_response";
inline constexpr TStringBuf FRONTEND_VH_EPISODES_REQUEST_RTLOG_TOKEN_ITEM = "hw_frontend_vh_episodes_request_rtlog_token";
inline constexpr TStringBuf FRONTEND_VH_PLAYER_REQUEST_ITEM = "hw_frontend_vh_player_request";
inline constexpr TStringBuf FRONTEND_VH_PLAYER_RESPONSE_ITEM = "hw_frontend_vh_player_response";
inline constexpr TStringBuf FRONTEND_VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM = "hw_frontend_vh_player_request_rtlog_token";

inline constexpr TStringBuf FALLBACK_SEASONS_RESPONSE = "fallback_seasons_response";
inline constexpr TStringBuf FLAG_USE_SEASONS_RESPONSE = "use_seasons_response";


inline TVideoPlaySceneArgs MakeVideoPlaySceneArgsFromVhResponse(const NVideoCommon::TVideoItemHelper& videoItemHelper, const TScenarioRunRequestWrapper request) {
    auto vhDirective = videoItemHelper.MakeVideoPlayDirective(request);
    TVideoPlaySceneArgs args;
    auto* additionalData = args.MutableAdditionalVhPlayerData();

    additionalData->SetHasActiveLicenses(videoItemHelper.GetHasActiveLicense());
    *additionalData->MutablePayload() = std::move(*vhDirective.MutablePayload());
    additionalData->SetStartAt(vhDirective.GetStartAt());
    additionalData->SetSessionToken(std::move(*vhDirective.MutableSessionToken()));
    *additionalData->MutableAudioLanguage() = std::move(*vhDirective.MutableAudioLanguage());
    *additionalData->MutableSubtitlesLanguage() = std::move(*vhDirective.MutableSubtitlesLanguage());
    if (videoItemHelper.GetPlayableVhPlayerData().VideoType == NVideoCommon::EContentType::TvShowEpisode) {
        *args.MutableTvShowItem() = std::move(*vhDirective.MutableTvShowItem());
    }
    *args.MutableVideoItem() = std::move(*vhDirective.MutableItem());
    return args;
}

std::pair<TStringBuf, TStringBuf> SelectFromAndServiceParams(const TScenarioRunRequestWrapper& request) {
    if (auto mediaDeviceIdentifier = TEnvironmentStateHelper{request}.FindMediaDeviceIdentifier();
    mediaDeviceIdentifier && mediaDeviceIdentifier->HasStrmFrom() && mediaDeviceIdentifier->HasOttServiceName()) {
        return {mediaDeviceIdentifier->GetStrmFrom(), mediaDeviceIdentifier->GetOttServiceName()};
    } else if (IsTvOrModuleOrTandemRequest(request)) {
        return {NVideoCommon::TV_FROM_ID, NVideoCommon::TV_SERVICE};
    } else {
        return {NVideoCommon::QUASAR_FROM_ID, NVideoCommon::QUASAR_SERVICE};
    }
}

TVideoPlaySceneArgs MakeVideoPlaySceneArgsWithWarns(TVideoItem&& item, bool noSuchSeason, bool noSuchEpisode) {
    TVideoPlaySceneArgs args;
    *args.MutableVideoItem() = std::move(item);
    if (noSuchSeason) {
        args.SetNoSuchSeason(1);
    }
    if (noSuchEpisode) {
        args.SetNoSuchEpisode(1);
    }
    return args;
}

ui32 GetSeasonLimit(const TVideoVhArgs& args) {
    return args.GetHasSeason() ? args.GetSeason() : 20;
}

void ReturnVhPlayerRequest(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, const TVideoVhArgs& args) {
    auto fromAndService = SelectFromAndServiceParams(request);
    return AddHttpRequestItems(ctx,
                                NVideoCommon::PrepareFrontendVhPlayerRequest(args.GetProviderItemId(), request, ctx, fromAndService.first, fromAndService.second),
                                FRONTEND_VH_PLAYER_REQUEST_ITEM,
                                FRONTEND_VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM);
}

void TVideoVhProxyPrepare::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    const auto args =  GetOnlyProtoOrThrow<TVideoVhArgs>(ctx.ServiceCtx, VH_PROXY_REQUEST);
    auto fromAndService = SelectFromAndServiceParams(request);
    if ((!args.GetHasSeason() && !args.GetHasEpisode()) || args.GetContentType() == "movie" || args.GetContentType() == "video") {
            // default film play
            LOG_INFO(ctx.Ctx.Logger()) << "FrontendVhPlayer request created for providerItemId: "  << args.GetProviderItemId();
            return AddHttpRequestItems(ctx,
                                       NVideoCommon::PrepareFrontendVhPlayerRequest(args.GetProviderItemId(), request, ctx, fromAndService.first, fromAndService.second),
                                       FRONTEND_VH_PLAYER_REQUEST_ITEM,
                                       FRONTEND_VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM);
    } else {
        if (args.GetHasSeason()) {
            // search for this season and if no episode, play first
            LOG_INFO(ctx.Ctx.Logger()) << "FrontendVhSeasonsRequest request created for providerItemId: " << args.GetProviderItemId();
            return AddHttpRequestItems(ctx,
                                       NVideoCommon::PrepareFrontendVhSeasonsRequest(args.GetProviderItemId(), request, ctx, GetSeasonLimit(args), 0, fromAndService.first, fromAndService.second),
                                       FRONTEND_VH_SEASONS_REQUEST_ITEM,
                                       FRONTEND_VH_SEASONS_REQUEST_RTLOG_TOKEN_ITEM);
        } else {
            // if we have episode without season, get from /player last played season, and search for the episode there
            LOG_INFO(ctx.Ctx.Logger()) << "FrontendVhPlayerGetLast request created for providerItemId: "  << args.GetProviderItemId();
            return AddHttpRequestItems(ctx,
                                       NVideoCommon::PrepareFrontendVhPlayerRequest(args.GetProviderItemId(), request, ctx, fromAndService.first, fromAndService.second),
                                       FRONTEND_VH_GET_LAST_REQUEST_ITEM,
                                       FRONTEND_VH_GET_LAST_REQUEST_RTLOG_TOKEN_ITEM);
        }
    }
}

void TVideoVhPlayerGetLastProcess::Do(TScenarioHandleContext& ctx) const {
    const TMaybe<NJson::TJsonValue> vhResponse = RetireHttpResponseJsonMaybe(
        ctx,
        FRONTEND_VH_GET_LAST_RESPONSE_ITEM,
        FRONTEND_VH_GET_LAST_REQUEST_RTLOG_TOKEN_ITEM
    );

    const auto runRequestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{runRequestProto, ctx.ServiceCtx};

    if (!vhResponse) {
        LOG_ERR(ctx.Ctx.Logger()) << "No response from player";
        return ctx.ServiceCtx.AddProtobufItem(TVideoPlaySceneArgs{}, VH_PROXY_RESPONSE);
    }
    LOG_INFO(ctx.Ctx.Logger()) << "VhGetLast reqid: " << (*vhResponse)["user_data"]["req_id"].GetString();
    if (const auto lastWatchedSeason = vhResponse->GetValueByPath("content.includes.[0].season"); lastWatchedSeason && lastWatchedSeason->Has("id")) {
        const auto& id = (*lastWatchedSeason)["id"].GetString();
        LOG_INFO(ctx.Ctx.Logger()) << "Got season: " << (*lastWatchedSeason)["season_number"].GetUInteger() << " providerItemId: " << id << "\nRequesting episodes";

        const auto args =  GetOnlyProtoOrThrow<TVideoVhArgs>(ctx.ServiceCtx, VH_PROXY_REQUEST);
        auto fromAndService = SelectFromAndServiceParams(request);
        return AddHttpRequestItems(ctx,
                                    NVideoCommon::PrepareFrontendVhEpisodesRequest(id, request, ctx, args.GetEpisode(), 0, fromAndService.first, fromAndService.second),
                                    FRONTEND_VH_EPISODES_REQUEST_ITEM,
                                    FRONTEND_VH_EPISODES_REQUEST_RTLOG_TOKEN_ITEM);
    }

    const auto args =  GetOnlyProtoOrThrow<TVideoVhArgs>(ctx.ServiceCtx, VH_PROXY_REQUEST);
    LOG_WARN(ctx.Ctx.Logger()) << "Fallback player response";
    return ReturnVhPlayerRequest(ctx, request, args);
}

void TVideoVhSeasonsProcess::Do(TScenarioHandleContext& ctx) const {
    TVideoVhArgs args =  GetOnlyProtoOrThrow<TVideoVhArgs>(ctx.ServiceCtx, VH_PROXY_REQUEST);
    const TMaybe<NJson::TJsonValue> vhResponse = RetireHttpResponseJsonMaybe(
        ctx,
        FRONTEND_VH_SEASONS_RESPONSE_ITEM,
        FRONTEND_VH_SEASONS_REQUEST_RTLOG_TOKEN_ITEM
    );
    if (!vhResponse) {
        LOG_CRIT(ctx.Ctx.Logger()) << "Empty response from vh seasons for providerItemId: " << args.GetProviderItemId();
        return;
    }

    LOG_INFO(ctx.Ctx.Logger()) << "VhSeasons reqid: " << (*vhResponse)["user_data"]["req_id"].GetString();
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};


    if (auto seriesHelper = NVideoCommon::TVhSeriesHelper::TryMakeFromVhSeriesResponse(*vhResponse)) {
        LOG_DEBUG(ctx.Ctx.Logger()) << "Searching for " << args.GetSeason() << " season in json...";
        if (auto season = seriesHelper->GetSeasonByNumber(args.GetSeason())) {
            LOG_INFO(ctx.Ctx.Logger()) << "Found target season!";
            if (args.GetHasEpisode()) {
                if (season->ContainsEpisode(args.GetEpisode())) {
                    LOG_INFO(ctx.Ctx.Logger()) << "Season has target episode. Requesting series_episodes handle with id: " << season->ContentId;
                    auto fromAndService = SelectFromAndServiceParams(request);
                    const THttpProxyRequest vhRequest = NVideoCommon::PrepareFrontendVhEpisodesRequest(season->ContentId, request, ctx, season->EpisodesCount, 0, fromAndService.first, fromAndService.second);
                    return AddHttpRequestItems(ctx, vhRequest, FRONTEND_VH_EPISODES_REQUEST_ITEM, FRONTEND_VH_EPISODES_REQUEST_RTLOG_TOKEN_ITEM);
                } else if (auto serialVideoItem = seriesHelper->GetSeriesVideoItem()) {
                    // no such episode
                    LOG_INFO(ctx.Ctx.Logger()) << "No target episode in season. Will render content details with warn";
                    return ctx.ServiceCtx.AddProtobufItem(MakeVideoPlaySceneArgsWithWarns(std::move(*serialVideoItem), false, true), VH_PROXY_RESPONSE);
                } else {
                    LOG_ERR(ctx.Ctx.Logger()) << "No target episode in season, but could not create series videoitem in seriesHelper from: " << (*vhResponse)["series"].GetStringRobust();
                }
            } else {
                LOG_INFO(ctx.Ctx.Logger()) << "Episode is not specified. Playing season from the beginning.";
                if (auto helper = NVideoCommon::TVideoItemHelper::TryMakeFromVhResponse(season->FirstEpisode)) {
                    auto response = MakeVideoPlaySceneArgsFromVhResponse(*helper, request);
                    LOG_INFO(ctx.Ctx.Logger()) << "Response is ready: " << response.DebugString();
                    return ctx.ServiceCtx.AddProtobufItem(std::move(response), VH_PROXY_RESPONSE);
                } else {
                    LOG_ERROR(ctx.Ctx.Logger()) << "Could not create helper on first episode of season play";
                }
            }
        } else if (auto serialVideoItem = seriesHelper->GetSeriesVideoItem()) {
            // no such season
            LOG_INFO(ctx.Ctx.Logger()) << "No target season in seasons. Will render content details with warn";
            return ctx.ServiceCtx.AddProtobufItem(MakeVideoPlaySceneArgsWithWarns(std::move(*serialVideoItem), true, false), VH_PROXY_RESPONSE);
        } else {
            LOG_ERROR(ctx.Ctx.Logger()) << "No target season in seasons, but could not create series videoitem in seriesHelper from: " << (*vhResponse)["series"].GetStringRobust();
        }
    } else {
        LOG_ERR(ctx.Ctx.Logger()) << "Incorrect vh resonse. Could not create helper.";
    }

    LOG_INFO(ctx.Ctx.Logger()) << "Making fallback response to VH /player";
    return ReturnVhPlayerRequest(ctx, request, args);
}

void TVideoVhEpisodesProcess::Do(TScenarioHandleContext& ctx) const {
    TVideoVhArgs args =  GetOnlyProtoOrThrow<TVideoVhArgs>(ctx.ServiceCtx, VH_PROXY_REQUEST);
    const TMaybe<NJson::TJsonValue> vhResponse = RetireHttpResponseJsonMaybe(
        ctx,
        FRONTEND_VH_EPISODES_RESPONSE_ITEM,
        FRONTEND_VH_EPISODES_REQUEST_RTLOG_TOKEN_ITEM
    );
    if (!vhResponse) {
        LOG_CRIT(ctx.Ctx.Logger()) << "Empty response from vh seasons for providerItemId: " << args.GetProviderItemId();
        return;
    }

    LOG_INFO(ctx.Ctx.Logger()) << "VhEpisodes reqid: " << (*vhResponse)["user_data"]["req_id"].GetString();
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    if (auto episode = NVideoCommon::GetEpisodeFromVhEpisodesResponse(*vhResponse, args.GetEpisode())) {
        LOG_INFO(ctx.Ctx.Logger()) << "Found episode in episodes. Requesting player handle with id: " << episode->ContentId;
        auto fromAndService = SelectFromAndServiceParams(request);
        return AddHttpRequestItems(ctx,
                                    NVideoCommon::PrepareFrontendVhPlayerRequest(episode->ContentId, request, ctx, fromAndService.first, fromAndService.second),
                                    FRONTEND_VH_PLAYER_REQUEST_ITEM,
                                    FRONTEND_VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM);
    } else {
        LOG_ERR(ctx.Ctx.Logger()) << "No episode found or empty content_id for found episode. Redirecting to player with id" << args.GetProviderItemId();
        return ReturnVhPlayerRequest(ctx, request, args);
    }
}

void TVideoVhPlayerProcess::Do(TScenarioHandleContext& ctx) const {
    const TMaybe<NJson::TJsonValue> vhResponse = RetireHttpResponseJsonMaybe(
        ctx,
        FRONTEND_VH_PLAYER_RESPONSE_ITEM,
        FRONTEND_VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM
    );
    if (!vhResponse) {
        LOG_CRIT(ctx.Ctx.Logger()) << "No response from vh player";
        return ctx.ServiceCtx.AddProtobufItem(TVideoPlaySceneArgs{}, VH_PROXY_RESPONSE);
    }

    LOG_INFO(ctx.Ctx.Logger()) << "VhPlayer reqid: " << (*vhResponse)["user_data"]["req_id"].GetString();
    const auto videoItemHelper = NVideoCommon::TVideoItemHelper::TryMakeFromVhPlayerResponse(*vhResponse);
    if (!videoItemHelper) {
        LOG_ERROR(ctx.Ctx.Logger()) << "Unknown vh response: " << vhResponse->GetStringRobust();
        return ctx.ServiceCtx.AddProtobufItem(TVideoPlaySceneArgs{}, VH_PROXY_RESPONSE);
    }

    const auto runRequestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{runRequestProto, ctx.ServiceCtx};
    auto result = MakeVideoPlaySceneArgsFromVhResponse(*videoItemHelper, request);
    LOG_INFO(ctx.Ctx.Logger()) << "Resulted with args: " << result.ShortUtf8DebugString();
    return ctx.ServiceCtx.AddProtobufItem(std::move(result), VH_PROXY_RESPONSE);
}

} // namespace NAlice::NHollywood::NVideo
