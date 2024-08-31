#include "frontend_vh_requests.h"

#include <alice/library/network/headers.h>
#include <alice/library/video_common/hollywood_helpers/util.h>

using namespace NAlice::NHollywood;

namespace NAlice::NVideoCommon {


THttpProxyRequest PrepareFrontendVhPlayerRequest(const TString& vhUuid,
                                                 const TScenarioRunRequestWrapper& request,
                                                 const TScenarioHandleContext& ctx,
                                                 TStringBuf from,
                                                 TStringBuf service) {

    TProxyRequestBuilder requestBuilder(ctx, request);

    requestBuilder.AddCgiParam("synchronous_scheme", "1");
    requestBuilder.AddCgiParam("service", service);
    requestBuilder.AddCgiParam("from", from);

    requestBuilder.SetEndpoint(TString::Join(vhUuid, ".json"));
    requestBuilder.SetScheme(NAppHostHttp::THttpRequest::None);

    const auto& requestOptions = request.BaseRequestProto().GetOptions();
    requestBuilder.AddHeader(NAlice::NNetwork::HEADER_USER_AGENT, requestOptions.GetUserAgent());
    AddCodecHeadersIntoRequest(requestBuilder, request);

    return requestBuilder.Build();
}
THttpProxyRequest PrepareFrontendVhPlayerRequest(const TString& vhUuid,
                                                 const NHollywoodFw::TRunRequest& request,
                                                 TStringBuf from,
                                                 TStringBuf service) {

    TProxyRequestBuilder requestBuilder(request);

    requestBuilder.AddCgiParam("synchronous_scheme", "1");
    requestBuilder.AddCgiParam("service", service);
    requestBuilder.AddCgiParam("from", from);

    requestBuilder.SetEndpoint(TString::Join(vhUuid, ".json"));
    requestBuilder.SetScheme(NAppHostHttp::THttpRequest::None);

    const auto& requestOptions = request.GetRunRequest().GetBaseRequest().GetOptions();
    requestBuilder.AddHeader(NAlice::NNetwork::HEADER_USER_AGENT, requestOptions.GetUserAgent());
    AddCodecHeadersIntoRequest(requestBuilder, request.GetRunRequest().GetBaseRequest());

    return requestBuilder.Build();
}


THttpProxyRequest PrepareFrontendVhSeasonsRequest(const TString& vhUuid,
                                                  const TScenarioRunRequestWrapper& request,
                                                  const TScenarioHandleContext& ctx,
                                                  ui32 limit,
                                                  ui32 offset,
                                                  TStringBuf from,
                                                  TStringBuf service) {

    TProxyRequestBuilder requestBuilder(ctx, request);

    requestBuilder.AddCgiParam("offset", ToString(offset));
    requestBuilder.AddCgiParam("limit", ToString(limit));
    requestBuilder.AddCgiParam("series_id", vhUuid);
    requestBuilder.AddCgiParam("service", service);
    requestBuilder.AddCgiParam("from", from);

    requestBuilder.SetScheme(NAppHostHttp::THttpRequest::None);

    const auto& requestOptions = request.BaseRequestProto().GetOptions();
    requestBuilder.AddHeader(NAlice::NNetwork::HEADER_USER_AGENT, requestOptions.GetUserAgent());
    AddCodecHeadersIntoRequest(requestBuilder, request);

    return requestBuilder.Build();
}
THttpProxyRequest PrepareFrontendVhSeasonsRequest(const TString& vhUuid,
                                                  const NHollywoodFw::TRunRequest& request,
                                                  ui32 limit,
                                                  ui32 offset,
                                                  TStringBuf from,
                                                  TStringBuf service) {

    TProxyRequestBuilder requestBuilder(request);

    requestBuilder.AddCgiParam("offset", ToString(offset));
    requestBuilder.AddCgiParam("limit", ToString(limit));
    requestBuilder.AddCgiParam("series_id", vhUuid);
    requestBuilder.AddCgiParam("service", service);
    requestBuilder.AddCgiParam("from", from);

    requestBuilder.SetScheme(NAppHostHttp::THttpRequest::None);

    const auto& requestOptions = request.GetRunRequest().GetBaseRequest().GetOptions();
    requestBuilder.AddHeader(NAlice::NNetwork::HEADER_USER_AGENT, requestOptions.GetUserAgent());
    AddCodecHeadersIntoRequest(requestBuilder, request.GetRunRequest().GetBaseRequest());

    return requestBuilder.Build();
}

THttpProxyRequest PrepareFrontendVhEpisodesRequest(const TString& vhUuid,
                                                  const TScenarioRunRequestWrapper& request,
                                                  const TScenarioHandleContext& ctx,
                                                  ui32 limit,
                                                  ui32 offset,
                                                  TStringBuf from,
                                                  TStringBuf service) {

    TProxyRequestBuilder requestBuilder(ctx, request);

    requestBuilder.AddCgiParam("offset", ToString(offset));
    requestBuilder.AddCgiParam("limit", ToString(limit));
    requestBuilder.AddCgiParam("season_id", vhUuid);
    requestBuilder.AddCgiParam("service", service);
    requestBuilder.AddCgiParam("from", from);

    requestBuilder.SetScheme(NAppHostHttp::THttpRequest::None);

    const auto& requestOptions = request.BaseRequestProto().GetOptions();
    requestBuilder.AddHeader(NAlice::NNetwork::HEADER_USER_AGENT, requestOptions.GetUserAgent());
    AddCodecHeadersIntoRequest(requestBuilder, request);

    return requestBuilder.Build();
}
THttpProxyRequest PrepareFrontendVhEpisodesRequest(const TString& vhUuid,
                                                  const NHollywoodFw::TRunRequest& request,
                                                  ui32 limit,
                                                  ui32 offset,
                                                  TStringBuf from,
                                                  TStringBuf service) {

    TProxyRequestBuilder requestBuilder(request);

    requestBuilder.AddCgiParam("offset", ToString(offset));
    requestBuilder.AddCgiParam("limit", ToString(limit));
    requestBuilder.AddCgiParam("season_id", vhUuid);
    requestBuilder.AddCgiParam("service", service);
    requestBuilder.AddCgiParam("from", from);

    requestBuilder.SetScheme(NAppHostHttp::THttpRequest::None);

    const auto& requestOptions = request.GetRunRequest().GetBaseRequest().GetOptions();
    requestBuilder.AddHeader(NAlice::NNetwork::HEADER_USER_AGENT, requestOptions.GetUserAgent());
    AddCodecHeadersIntoRequest(requestBuilder, request.GetRunRequest().GetBaseRequest());

    return requestBuilder.Build();
}

} // namespace NAlice::NVideoCommon
