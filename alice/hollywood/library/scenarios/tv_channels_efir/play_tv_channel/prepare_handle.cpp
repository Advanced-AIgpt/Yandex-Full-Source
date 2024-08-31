#include "prepare_handle.h"

#include <alice/hollywood/library/scenarios/tv_channels_efir/library/util.h>

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/library/video_common/hollywood_helpers/proxy_request_builder.h>
#include <alice/library/video_common/hollywood_helpers/util.h>

#include <alice/library/network/headers.h>


using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NTvChannelsEfir {

namespace {

THttpProxyRequest PrepareYandexVideoResultRequest(const TScenarioRunRequestWrapper& request,
                                                     const TScenarioHandleContext& ctx,
                                                     const TString& channelName) {

    NVideoCommon::TProxyRequestBuilder requestBuilder(ctx, request);

    requestBuilder.AddCgiParam("text", channelName);

    requestBuilder.AddCgiParam("from", "tabbar");
    requestBuilder.AddCgiParam("restrict_config", "hubtag:ATTR_LITERAL");
    requestBuilder.AddCgiParam("wizextra", "add_video_search_attr=hubtag:hhkkxgvzufwokinhh");
    requestBuilder.AddCgiParam("relev", "pf=off");
    requestBuilder.AddCgiParam("relev", "station_request=1");

    requestBuilder.SetScheme(NAppHostHttp::THttpRequest::None);

    const auto& requestOptions = request.BaseRequestProto().GetOptions();
    requestBuilder.AddHeader(NNetwork::HEADER_USER_AGENT, requestOptions.GetUserAgent());
    requestBuilder.AddHeader(NNetwork::HEADER_HOST, "yandex.ru");
    requestBuilder.AddHeader(NNetwork::HEADER_X_YANDEX_INTERNAL_REQUEST, "1");
    requestBuilder.AddHeader(NNetwork::HEADER_X_FORWARDED_FOR, requestOptions.GetClientIP());

    return requestBuilder.Build();
}

} // namespace

void PlayTvChannelPrepareHandle(TScenarioHandleContext& ctx) {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    bool isTvPlugged = request.ClientInfo().IsSmartSpeaker() && request.BaseRequestProto().GetInterfaces().GetIsTvPlugged();
    if (!isTvPlugged) {
        NVideoCommon::AddIrrelevantResponse(ctx);
        return;
    }

    const auto channelName = GetChannelName(request);
    if (channelName.Empty()) {
        NVideoCommon::AddIrrelevantResponse(ctx);
        return;
    }

    const THttpProxyRequest quasarRequest = PrepareYandexVideoResultRequest(request, ctx, channelName);
    AddHttpRequestItems(ctx, quasarRequest, VIDEO_RESULT_REQUEST_ITEM, VIDEO_RESULT_REQUEST_RTLOG_TOKEN_ITEM);
}

} // namespace NAlice::NHollywood::NTvChannelsEfir
