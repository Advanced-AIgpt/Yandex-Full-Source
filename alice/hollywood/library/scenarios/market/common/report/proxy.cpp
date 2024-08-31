#include "proxy.h"

#include <alice/hollywood/library/scenarios/market/common/proto/report.pb.h>

#include <alice/hollywood/library/http_proxy/http_proxy.h>

namespace NAlice::NHollywood::NMarket {

namespace {

constexpr TStringBuf SEARCH_INFO_KEY = "market_search_info";
constexpr TStringBuf REPORT_CLIENT_KEY = "market_report_client";
constexpr TStringBuf REPORT_REQUEST_KEY = "market_report_request";
constexpr TStringBuf REPORT_RESPONSE_KEY = "market_report_response";

} // namespace

void AddSearchInfoRequest(
    const TSearchInfo& searchInfo,
    TReportClient& client,
    TMarketBaseContext& ctx,
    bool allowRedirects)
{
    auto request = searchInfo.CreateReportRequest(client);
    request.SetAllowRedirects(allowRedirects);
    AddReportRequest(request, ctx);
    AddSearchInfo(searchInfo, ctx);
    AddReportClient(client, ctx);
}

void AddSearchInfo(const TSearchInfo& searchInfo, TMarketBaseContext& ctx)
{
    const auto proto = searchInfo.ToProto();
    LOG_INFO(ctx.Logger())
        << "add search info item. key: \"" << REPORT_CLIENT_KEY << "\" item: " << proto;
    ctx->ServiceCtx.AddProtobufItem(proto, SEARCH_INFO_KEY);
}

void AddReportClient(const TReportClient& client, TMarketBaseContext& ctx)
{
    const auto proto = client.CreateProto();
    LOG_INFO(ctx.Logger())
        << "add report client item. key: \"" << REPORT_CLIENT_KEY << "\" item: " << proto;
    ctx->ServiceCtx.AddProtobufItem(proto, REPORT_CLIENT_KEY);
}

void AddReportRequest(const TReportPrimeRequest& request, TMarketBaseContext& ctx)
{
    const TString path = request.Path();
    const THttpProxyRequest appHostRequest = PrepareHttpRequest(
        path,
        ctx->RequestMeta,
        ctx.Logger(),
        "" /* = name */,
        Nothing() /* = body */,
        NAppHostHttp::THttpRequest::Get,
        request.Headers());
    AddHttpRequestItems(*ctx, appHostRequest, REPORT_REQUEST_KEY);
}

TSearchInfo GetSearchInfoOrThrow(const TMarketBaseContext& ctx)
{
    auto proto = GetOnlyProtoOrThrow<NProto::TSearchInfo>(ctx->ServiceCtx, SEARCH_INFO_KEY);
    LOG_INFO(ctx.Logger()) << "search info proto " << proto;
    return { proto };
}

TReportClient GetReportClientOrThrow(const TMarketBaseContext& ctx)
{
    auto proto = GetOnlyProtoOrThrow<NProto::TReportClient>(ctx->ServiceCtx, REPORT_CLIENT_KEY);
    LOG_INFO(ctx.Logger()) << "report client proto " << proto;
    return { proto };
}

TReportPrimeResponse RetireReportPrimeResponse(const TMarketBaseContext& ctx)
{
    auto data = RetireHttpResponseJson(
        *ctx,
        REPORT_RESPONSE_KEY,
        PROXY_RTLOG_TOKEN_KEY_DEFAULT,
        false /* = logBody */);
    return { data };
}

} // namespace NAlice::NHollywood::NMarket
