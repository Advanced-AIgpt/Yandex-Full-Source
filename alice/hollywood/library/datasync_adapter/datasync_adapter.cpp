#include "datasync_adapter.h"

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf DATASYNC_REQUEST_ITEM = "hw_datasync_request";
constexpr TStringBuf DATASYNC_REQUEST_RTLOG_TOKEN_ITEM = "hw_datasync_request_rtlog_token";
constexpr TStringBuf DATASYNC_RESPONSE_ITEM = "hw_datasync_response";

} // namespace

THttpProxyRequest PrepareDataSyncRequest(const TStringBuf path,
                                         const NScenarios::TRequestMeta& meta,
                                         const TString& uid,
                                         TRTLogger& logger,
                                         const TString& name,
                                         const TMaybe<TString> body,
                                         const TMaybe<NAppHostHttp::THttpRequest_EMethod> maybeMethod)
{
    auto request = PrepareHttpRequest(path, meta, logger, name, body, maybeMethod);
    auto& header = *request.Request.AddHeaders();
    header.SetName("X-Uid");
    header.SetValue(uid);
    return request;
}

void AddDataSyncRequestItems(TScenarioHandleContext& ctx, const THttpProxyRequest& request) {
    AddHttpRequestItems(ctx, request, DATASYNC_REQUEST_ITEM, DATASYNC_REQUEST_RTLOG_TOKEN_ITEM);
}

NJson::TJsonValue RetireDataSyncResponseItems(const TScenarioHandleContext& ctx) {
    return RetireHttpResponseJson(ctx, DATASYNC_RESPONSE_ITEM, DATASYNC_REQUEST_RTLOG_TOKEN_ITEM);
}

TMaybe<NJson::TJsonValue> RetireDataSyncResponseItemsSafe(const TScenarioHandleContext& ctx) {
    return RetireHttpResponseJsonMaybe(ctx, DATASYNC_RESPONSE_ITEM, DATASYNC_REQUEST_RTLOG_TOKEN_ITEM);
}

} // namespace NAlice::NHollywood
