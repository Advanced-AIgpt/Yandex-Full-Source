#include "ott_setup.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/mordovia_video_selection/util.h>
#include <alice/library/video_common/hollywood_helpers/proxy_request_builder.h>
#include <alice/library/video_common/frontend_vh_helpers/video_item_helper.h>

#include <alice/library/network/headers.h>
#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/device_helpers.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>

using namespace NAlice::NScenarios;
using NAlice::NVideoCommon::EContentType;

namespace NAlice::NHollywood::NMordovia {

namespace {

constexpr TStringBuf STATION_SERVICE_ID = "42";

TMaybe<TString> GetSupportedAudioCodecs(const TInterfaces& interfaces) {
    TString audioCodecs = "";

    if (interfaces.GetAudioCodecAAC()) {
        audioCodecs += "AAC,";
    }

    if (interfaces.GetAudioCodecAC3()) {
        audioCodecs += "AC3,";
    }

    if (interfaces.GetAudioCodecEAC3()) {
        audioCodecs += "EAC3,";
    }

    if (interfaces.GetAudioCodecVORBIS()) {
        audioCodecs += "VORBIS,";
    }

    if (interfaces.GetAudioCodecOPUS()) {
        audioCodecs += "OPUS,";
    }

    if (audioCodecs.size() > 0) {
        return audioCodecs.substr(0, audioCodecs.size() - 1);
    }

    return {};
}

TMaybe<THttpProxyRequest> PrepareOttContentStreamRequest(const NVideoCommon::TVideoItemHelper& videoItemHelper,
                                                 const TScenarioRunRequestWrapper& request,
                                                 const TScenarioHandleContext& ctx) {

    const auto& vhData = videoItemHelper.GetPlayableVhPlayerData();
    const auto& uuid = vhData.Uuid;
    if (uuid.Empty() || (vhData.VideoType != EContentType::Movie && vhData.VideoType != EContentType::TvShowEpisode)) {
        return Nothing();
    }
    const auto& itemType = vhData.VideoType == EContentType::Movie ? "MOVIE" : "TV_SHOW_EPISODE";

    NVideoCommon::TProxyRequestBuilder requestBuilder(ctx, request);

    requestBuilder.AddCgiParam("serviceId", STATION_SERVICE_ID);
    TString path = TStringBuilder() << "content/" << itemType << ":" << uuid << "/stream";

    requestBuilder.SetEndpoint(path);
    requestBuilder.SetScheme(NAppHostHttp::THttpRequest::None);

    const auto& requestOptions = request.BaseRequestProto().GetOptions();
    requestBuilder.AddHeader(NAlice::NNetwork::HEADER_USER_AGENT, requestOptions.GetUserAgent());
    requestBuilder.AddHeader(NAlice::NNetwork::HEADER_X_FORWARDED_FOR, requestOptions.GetClientIP());

    if (const auto audioCodecs = GetSupportedAudioCodecs(request.Interfaces())) {
        requestBuilder.AddHeader(NAlice::NNetwork::HEADER_X_DEVICE_AUDIO_CODECS, ToString(audioCodecs));
    }
    return requestBuilder.Build();
}

}

void TOttSetup::Do(TScenarioHandleContext& ctx) const {
    const auto vhResponse =
        RetireHttpResponseJsonMaybe(ctx, FRONTEND_VH_PLAYER_RESPONSE_ITEM, FRONTEND_VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM);
    if (!vhResponse) {
        return;
    }

    auto videoItemHelper = NVideoCommon::TVideoItemHelper::TryMakeFromVhPlayerResponse(*vhResponse);
    if (!videoItemHelper) {
        return;
    }

    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    const auto ottRequest = PrepareOttContentStreamRequest(*videoItemHelper, request, ctx);
    if (!ottRequest) {
        return;
    }
    AddHttpRequestItems(ctx, *ottRequest, OTT_STREAMS_META_REQUEST_ITEM, OTT_STREAMS_META_REQUEST_RTLOG_TOKEN_ITEM);
}

} // namespace NAlice::NHollywood::NMordovia
