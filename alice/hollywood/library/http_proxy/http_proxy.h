#pragma once

#include "request_meta_provider.h"

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/protos/bass_request_rtlog_token.pb.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>
#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>
#include <alice/library/network/common.h>

#include <library/cpp/http/io/headers.h>
#include <util/generic/maybe.h>
#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NHollywood {

constexpr TStringBuf PROXY_REQUEST_KEY_DEFAULT = "http_request";
constexpr TStringBuf PROXY_RESPONSE_KEY_DEFAULT = "http_response";
constexpr TStringBuf PROXY_RTLOG_TOKEN_KEY_DEFAULT = "request_rtlog_token";

TMaybe<THttpInputHeader> GetBalancingHintHeader(const TStringBuf currentDc);

struct THttpProxyRequest {
    NAppHostHttp::THttpRequest Request;
    TBassRequestRTLogToken RTLogToken;
};

TMaybe<NJson::TJsonValue> RetireJsonResponseMaybe(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    const TBassRequestRTLogToken& proxyRequestRTLogToken,
    TRTLogger& logger,
    bool logBody,
    bool throwOnFailure);

// removing rtlog
TMaybe<NJson::TJsonValue> RetireJsonResponseMaybe(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    TRTLogger& logger,
    bool logBody,
    bool throwOnFailure);

TMaybe<TString> RetireResponseMaybe(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    const TBassRequestRTLogToken& proxyRequestRTLogToken,
    TRTLogger& logger,
    bool logBody,
    bool throwOnFailure);

// removing rtlog
TMaybe<TString> RetireResponseMaybe(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    TRTLogger& logger,
    bool logBody,
    bool throwOnFailure);

void AddHttpRequestItems(
    TScenarioHandleContext& ctx,
    const THttpProxyRequest& proxyRequest,
    TStringBuf requestKey = PROXY_REQUEST_KEY_DEFAULT,
    TStringBuf tokenKey = PROXY_RTLOG_TOKEN_KEY_DEFAULT);

// removing rtlog
void AddHttpRequestItems(
    TScenarioHandleContext& ctx,
    const NAppHostHttp::THttpRequest& proxyRequest,
    TStringBuf requestKey = PROXY_REQUEST_KEY_DEFAULT);

NJson::TJsonValue RetireHttpResponseJson(
    const TScenarioHandleContext& ctx,
    TStringBuf responseKey = PROXY_RESPONSE_KEY_DEFAULT,
    TStringBuf tokenKey = PROXY_RTLOG_TOKEN_KEY_DEFAULT,
    bool logBody = true);

TMaybe<NJson::TJsonValue> RetireHttpResponseJsonMaybe(
    const TScenarioHandleContext& ctx,
    TStringBuf responseKey = PROXY_RESPONSE_KEY_DEFAULT,
    TStringBuf tokenKey = PROXY_RTLOG_TOKEN_KEY_DEFAULT,
    bool logBody = true);

TString RetireResponse(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    const TBassRequestRTLogToken& proxyRequestRTLogToken,
    TRTLogger& logger,
    bool logBody = true);

// removing rtlog
TString RetireResponse(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    TRTLogger& logger,
    bool logBody = true);

template<typename TResponse>
TResponse RetireProtoResponse(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    const TBassRequestRTLogToken& proxyRequestRTLogToken,
    TRTLogger& logger)
{
    return ParseProto<TResponse>(RetireResponse(std::move(response), proxyRequestRTLogToken, logger, false));
}

// removing rtlog
template<typename TResponse>
TResponse RetireProtoResponse(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    TRTLogger& logger)
{
    return ParseProto<TResponse>(RetireResponse(std::move(response), logger, false));
}

template<typename TResponse>
TResponse RetireHttpResponseProto(
    const TScenarioHandleContext& ctx,
    TStringBuf responseKey = PROXY_RESPONSE_KEY_DEFAULT,
    TStringBuf tokenKey = PROXY_RTLOG_TOKEN_KEY_DEFAULT)
{
    Y_UNUSED(tokenKey);
    const TBassRequestRTLogToken requestRTLogToken;
    auto maybeResponse = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(ctx.ServiceCtx, responseKey);

    return RetireProtoResponse<TResponse>(std::move(maybeResponse), requestRTLogToken, ctx.Ctx.Logger());
}

template<typename TResponse>
TMaybe<TResponse> RetireProtoResponseMaybe(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    const TBassRequestRTLogToken& proxyRequestRTLogToken,
    TRTLogger& logger)
{
    TMaybe<TString> reponseStr = RetireResponseMaybe(std::move(response), proxyRequestRTLogToken, logger, false, false);
    if (!reponseStr.Defined()) {
        return Nothing();
    }
    return ParseProto<TResponse>(*reponseStr);
}

// removing rtlog
template<typename TResponse>
TMaybe<TResponse> RetireProtoResponseMaybe(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    TRTLogger& logger)
{
    TMaybe<TString> reponseStr = RetireResponseMaybe(std::move(response), logger, false, false);
    if (!reponseStr.Defined()) {
        return Nothing();
    }
    return ParseProto<TResponse>(*reponseStr);
}

template<typename TResponse>
TMaybe<TResponse> RetireHttpResponseProtoMaybe(
    const TScenarioHandleContext& ctx,
    TStringBuf responseKey = PROXY_RESPONSE_KEY_DEFAULT,
    TStringBuf tokenKey = PROXY_RTLOG_TOKEN_KEY_DEFAULT)
{
    Y_UNUSED(tokenKey);
    const TBassRequestRTLogToken requestRTLogToken;

    auto maybeResponse = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(ctx.ServiceCtx, responseKey);

    return RetireProtoResponseMaybe<TResponse>(std::move(maybeResponse), requestRTLogToken, ctx.Ctx.Logger());
}

// TODO(vitvlkv): Remove it, better use THttpProxyRequestBuilder instead.
// We leave this func as is for now, because we do not want to break hollywood scenarios that rely on it...
THttpProxyRequest PrepareHttpRequest(const TStringBuf path,
                                     const NScenarios::TRequestMeta& meta,
                                     TRTLogger& logger,
                                     const TString& name = Default<TString>(),
                                     const TMaybe<TString> body = Nothing(),
                                     const TMaybe<NAppHostHttp::THttpRequest_EMethod> maybeMethod = Nothing(),
                                     const THttpHeaders& headers = Default<THttpHeaders>(),
                                     bool useOAuth = true, bool useTVMUserTicket = true,
                                     const TString& oauthTokenPrefix = "OAuth");

// removing rtlog
// TODO(vitvlkv): Remove it, better use THttpProxyRequestBuilder instead.
// We leave this func as is for now, because we do not want to break hollywood scenarios that rely on it...
NAppHostHttp::THttpRequest PrepareNoRtlogHttpRequest(const TStringBuf path,
                                     const NScenarios::TRequestMeta& meta,
                                     TRTLogger& logger,
                                     const TString& name = Default<TString>(),
                                     const TMaybe<TString> body = Nothing(),
                                     const TMaybe<NAppHostHttp::THttpRequest_EMethod> maybeMethod = Nothing(),
                                     const THttpHeaders& headers = Default<THttpHeaders>(),
                                     bool useOAuth = true, bool useTVMUserTicket = true,
                                     const TString& oauthTokenPrefix = "OAuth");

class THttpProxyRequestBuilder {
public:
    // Param name is optional, but for the sake of better logging use some good name for your request
    THttpProxyRequestBuilder(const TStringBuf path, TAtomicSharedPtr<IRequestMetaProvider> metaProvider, TRTLogger& logger,
                             const TString name = Default<TString>());

    THttpProxyRequestBuilder(const TStringBuf path, const NScenarios::TRequestMeta& meta, TRTLogger& logger,
                             const TString name = Default<TString>());

    // Optional
    THttpProxyRequestBuilder& SetBody(const TString& body,
                                      const TStringBuf contentType = NContentTypes::APPLICATION_JSON);

    // Optional, GET by default
    THttpProxyRequestBuilder& SetMethod(NAppHostHttp::THttpRequest_EMethod method);

    // Optional, Http by default
    THttpProxyRequestBuilder& SetScheme(NAppHostHttp::THttpRequest_EScheme scheme);

    THttpProxyRequestBuilder& AddHeader(const TStringBuf name, const TStringBuf value);

    THttpProxyRequestBuilder& AddHeaders(const THttpHeaders& headers);

    THttpProxyRequestBuilder& SetUseOAuth(const TString& oauthTokenPrefix = "OAuth");

    THttpProxyRequestBuilder& SetUseTVMUserTicket();

    THttpProxyRequestBuilder& AddBalancingHint();

    THttpProxyRequest Build(const bool logVerbose = false);

    THttpProxyRequest BuildAndMove(const bool logVerbose = false) &&;

    static void SetCurrentDc(const TString& currentDc);

private:
    static TString CurrentDc_;

    THttpProxyRequest ProxyRequest_;
    TAtomicSharedPtr<IRequestMetaProvider> MetaProvider_;
    TRTLogger& Logger_;
    const TString Name_;
};

class THttpProxyNoRtlogRequestBuilder {
public:
    // Param name is optional, but for the sake of better logging use some good name for your request
    THttpProxyNoRtlogRequestBuilder(const TStringBuf path, TAtomicSharedPtr<IRequestMetaProvider> metaProvider, TRTLogger& logger,
                                    const TString name = Default<TString>(), bool randomizeRequestId = false);

    // Param name is optional, but for the sake of better logging use some good name for your request
    THttpProxyNoRtlogRequestBuilder(const TStringBuf path, const NScenarios::TRequestMeta& meta, TRTLogger& logger,
                                    const TString name = Default<TString>(), bool randomizeRequestId = false);

    // Optional
    THttpProxyNoRtlogRequestBuilder& SetBody(const TString& body,
                                      const TStringBuf contentType = NContentTypes::APPLICATION_JSON);

    // Optional, GET by default
    THttpProxyNoRtlogRequestBuilder& SetMethod(NAppHostHttp::THttpRequest_EMethod method);

    // Optional, Http by default
    THttpProxyNoRtlogRequestBuilder& SetScheme(NAppHostHttp::THttpRequest_EScheme scheme);

    THttpProxyNoRtlogRequestBuilder& AddHeader(const TStringBuf name, const TStringBuf value);

    THttpProxyNoRtlogRequestBuilder& AddHeaders(const THttpHeaders& headers);

    THttpProxyNoRtlogRequestBuilder& SetUseOAuth(const TString& oauthTokenPrefix = "OAuth");

    THttpProxyNoRtlogRequestBuilder& SetUseTVMUserTicket();

    THttpProxyNoRtlogRequestBuilder& AddBalancingHint();

    NAppHostHttp::THttpRequest Build(const bool logVerbose = false);

    NAppHostHttp::THttpRequest BuildAndMove(const bool logVerbose = false) &&;

    static void SetCurrentDc(const TString& currentDc);

private:

    static TString CurrentDc_;

    NAppHostHttp::THttpRequest ProxyRequest_;
    TAtomicSharedPtr<IRequestMetaProvider> MetaProvider_;
    TRTLogger& Logger_;
    const TString Name_;
};

} // namespace NAlice::NHollywood
