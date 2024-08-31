#include "entity_search_adapter.h"

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf ENTITY_SEARCH_REQUEST_ITEM = "hw_entity_search_request";
constexpr TStringBuf ENTITY_SEARCH_REQUEST_RTLOG_TOKEN_ITEM = "hw_entity_search_request_rtlog_token";
constexpr TStringBuf ENTITY_SEARCH_RESPONSE_ITEM = "hw_entity_search_response";

} // namespace

THttpProxyRequest PrepareEntitySearchRequest(const TStringBuf path,
                                             const NScenarios::TRequestMeta& meta,
                                             TRTLogger& logger,
                                             const TString& name)
{
    return PrepareHttpRequest(path, meta, logger, name);
}

void AddEntitySearchRequestItems(TScenarioHandleContext& ctx, const THttpProxyRequest& request) {
    AddHttpRequestItems(ctx, request, ENTITY_SEARCH_REQUEST_ITEM, ENTITY_SEARCH_REQUEST_RTLOG_TOKEN_ITEM);
}

NJson::TJsonValue RetireEntitySearchResponseItems(const TScenarioHandleContext& ctx) {
    return RetireHttpResponseJson(ctx, ENTITY_SEARCH_RESPONSE_ITEM, ENTITY_SEARCH_REQUEST_RTLOG_TOKEN_ITEM);
}

} // namespace NAlice::NHollywood
