#include "http_proxy.h"

#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/library/network/headers.h>

#include <util/generic/maybe.h>
#include <util/string/builder.h>
#include <util/string/cast.h>

namespace NAlice::NHollywood {

constexpr std::array<TStringBuf, 3> ALLOWED_DCS_FOR_BALANCING_HIT = {{"man", "sas", "vla"}};

namespace {

TString CreateChildRTLogToken(TRTLogger& logger, const TString& name) {
    return logger.RequestLogger()->LogChildActivationStarted(false, name);
}

void LogRequest(TRTLogger& logger, const TStringBuf name, const NAppHostHttp::THttpRequest& request, const bool verbose = false) {
    auto logMsg = TStringBuilder() << "Request prepared, name: " << name << ", path: " << request.GetPath();
    if (verbose && !request.GetContent().empty()) {
        logMsg << ", body: " << request.GetContent();
    }
    LOG_INFO(logger) << logMsg;
}

} // anonymous namespace

// TODO(vitvlkv): Remove this function in favor of THttpProxyRequestBuilder::AddBalancingHint()
TMaybe<THttpInputHeader> GetBalancingHintHeader(const TStringBuf currentDc) {
    if (IsIn(ALLOWED_DCS_FOR_BALANCING_HIT, currentDc)) {
        return THttpInputHeader(TString(NNetwork::HEADER_X_BALANCER_DC_HINT), TString(currentDc));
    }
    return Nothing();
}

TMaybe<TString> RetireResponseMaybe(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    const TBassRequestRTLogToken& proxyRequestRTLogToken,
    TRTLogger& logger,
    bool logBody,
    bool throwOnFailure)
{
    Y_UNUSED(proxyRequestRTLogToken);

    if (!response) {
        const TStringBuf message = "Request failed, no response was received";
        LOG_ERROR(logger) << message;
        if (throwOnFailure) {
            ythrow yexception() << message;
        }
        return Nothing();
    }
    const bool success = (response->GetStatusCode() >= 200 && response->GetStatusCode() < 300);
    LOG_INFO(logger) << "Request "
                     << (success ? "succeded" : "failed")
                     << " with the status code: " << response->GetStatusCode()
                     << (logBody ? ", body: " + response->GetContent() : "");

    if (!success) {
        const TStringBuf message = "Response failed";
        LOG_ERROR(logger) << message;
        if (throwOnFailure) {
            ythrow yexception() << message;
        }
        return Nothing();
    }

    return std::move(*response->MutableContent());
}

// removing rtlog
TMaybe<TString> RetireResponseMaybe(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    TRTLogger& logger,
    bool logBody,
    bool throwOnFailure)
{

    if (!response) {
        const TStringBuf message = "Request failed, no response was received";
        LOG_ERROR(logger) << message;
        if (throwOnFailure) {
            ythrow yexception() << message;
        }
        return Nothing();
    }
    const bool success = (response->GetStatusCode() >= 200 && response->GetStatusCode() < 300);
    LOG_INFO(logger) << "Request "
                     << (success ? "succeded" : "failed")
                     << " with the status code: " << response->GetStatusCode()
                     << (logBody ? ", body: " + response->GetContent() : "");

    if (!success) {
        const TStringBuf message = "Response failed";
        LOG_ERROR(logger) << message;
        if (throwOnFailure) {
            ythrow yexception() << message;
        }
        return Nothing();
    }

    return std::move(*response->MutableContent());
}

TMaybe<NJson::TJsonValue> RetireJsonResponseMaybe(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    const TBassRequestRTLogToken& proxyRequestRTLogToken,
    TRTLogger& logger,
    bool logBody,
    bool throwOnFailure)
{
    const auto& responseStringMaybe = RetireResponseMaybe(std::move(response), proxyRequestRTLogToken,
        logger, logBody, throwOnFailure);

    if (responseStringMaybe) {
        return JsonFromString(responseStringMaybe.GetRef());
    }

    return Nothing();
}

// removing rtlog
TMaybe<NJson::TJsonValue> RetireJsonResponseMaybe(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    TRTLogger& logger,
    bool logBody,
    bool throwOnFailure)
{
    const auto& responseStringMaybe = RetireResponseMaybe(std::move(response),
        logger, logBody, throwOnFailure);

    if (responseStringMaybe) {
        return JsonFromString(responseStringMaybe.GetRef());
    }

    return Nothing();
}

TString RetireResponse(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    const TBassRequestRTLogToken& proxyRequestRTLogToken,
    TRTLogger& logger,
    bool logBody)
{
    auto responseStringMaybe = RetireResponseMaybe(std::move(response), proxyRequestRTLogToken,
        logger, logBody, /* throwOnFailure = */ true);
    return std::move(responseStringMaybe.GetRef());
}

//removing rtlog
TString RetireResponse(
    TMaybe<NAppHostHttp::THttpResponse>&& response,
    TRTLogger& logger,
    bool logBody)
{
    auto responseStringMaybe = RetireResponseMaybe(std::move(response),
        logger, logBody, /* throwOnFailure = */ true);
    return std::move(responseStringMaybe.GetRef());
}

void AddHttpRequestItems(
    TScenarioHandleContext& ctx,
    const THttpProxyRequest& proxyRequest,
    TStringBuf requestKey,
    TStringBuf tokenKey)
{
    ctx.ServiceCtx.AddProtobufItem(proxyRequest.Request, requestKey);
    ctx.ServiceCtx.AddProtobufItem(proxyRequest.RTLogToken, tokenKey);
    LOG_INFO(ctx.Ctx.Logger()) << "Request item added with request key: " << requestKey << ", token key: " << tokenKey;
}

// removing rtlog
void AddHttpRequestItems(
    TScenarioHandleContext& ctx,
    const NAppHostHttp::THttpRequest& proxyRequest,
    TStringBuf requestKey)
{
    ctx.ServiceCtx.AddProtobufItem(proxyRequest, requestKey);
    LOG_INFO(ctx.Ctx.Logger()) << "Request item added with request key: " << requestKey;
}

NJson::TJsonValue RetireHttpResponseJson(
    const TScenarioHandleContext& ctx,
    TStringBuf responseKey,
    TStringBuf tokenKey,
    bool logBody)
{
    Y_UNUSED(tokenKey);
    const TBassRequestRTLogToken requestRTLogToken;
    auto maybeResponse = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(ctx.ServiceCtx, responseKey);
    auto responseStringMaybe = RetireJsonResponseMaybe(std::move(maybeResponse), requestRTLogToken,
        ctx.Ctx.Logger(), logBody, /* throwOnFailure = */ true);

    return std::move(*responseStringMaybe);
}

TMaybe<NJson::TJsonValue> RetireHttpResponseJsonMaybe(
    const TScenarioHandleContext& ctx,
    TStringBuf responseKey,
    TStringBuf tokenKey,
    bool logBody)
{
    Y_UNUSED(tokenKey);
    const TBassRequestRTLogToken requestRTLogToken;

    auto maybeResponse = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(ctx.ServiceCtx, responseKey);

    return RetireJsonResponseMaybe(std::move(maybeResponse), requestRTLogToken, ctx.Ctx.Logger(),
        logBody, /* throwOnFailure = */ false);
}

THttpProxyRequest PrepareHttpRequest(const TStringBuf path,
                                     const NScenarios::TRequestMeta& meta,
                                     TRTLogger& logger,
                                     const TString& name,
                                     const TMaybe<TString> body,
                                     const TMaybe<NAppHostHttp::THttpRequest_EMethod> maybeMethod,
                                     const THttpHeaders& headers,
                                     bool useOAuth,
                                     bool useTVMUserTicket,
                                     const TString& oauthTokenPrefix)
{
    THttpProxyRequest proxyRequest;
    NAppHostHttp::THttpRequest& request = proxyRequest.Request;
    request.SetScheme(NAppHostHttp::THttpRequest::Http);

    NAppHostHttp::THttpRequest_EMethod method;
    if (maybeMethod) {
        method = *maybeMethod;
    } else if (body) {
        method = NAppHostHttp::THttpRequest::Post;
    } else {
        method = NAppHostHttp::THttpRequest::Get;
    }
    request.SetMethod(method);
    if (body && (method == NAppHostHttp::THttpRequest::Post || method == NAppHostHttp::THttpRequest::Put)) {
        request.SetContent(body.GetRef());
    }
    request.SetPath(ToString(path));

    auto addHeader = [&request](const TStringBuf name, const TStringBuf value) {
        auto& header = *request.AddHeaders();
        header.SetName(TString{name});
        header.SetValue(TString{value});
    };

    for (const auto& header : headers) {
        addHeader(header.Name(), header.Value());
    }

    {
        const auto childRTLogToken = CreateChildRTLogToken(logger, name);
        proxyRequest.RTLogToken.SetRTLogToken(childRTLogToken);
        addHeader(NNetwork::HEADER_X_RTLOG_TOKEN, childRTLogToken);
    }

    addHeader(NNetwork::HEADER_CONTENT_TYPE, NContentTypes::APPLICATION_JSON);
    addHeader(NNetwork::HEADER_X_REQUEST_ID, meta.GetRequestId());

    if (const auto& userTicket = meta.GetUserTicket(); useTVMUserTicket && !userTicket.empty()) {
        addHeader(NNetwork::HEADER_X_YA_USER_TICKET, userTicket);
    }

    if (const auto& oauthToken = meta.GetOAuthToken(); useOAuth && !oauthToken.empty()) {
        addHeader("Authorization", oauthTokenPrefix + " " + oauthToken);
    }

    LogRequest(logger, name, request);

    return proxyRequest;
}

THttpProxyRequestBuilder::THttpProxyRequestBuilder(const TStringBuf path, TAtomicSharedPtr<IRequestMetaProvider> metaProvider, TRTLogger& logger,
                                                   const TString name)
    : ProxyRequest_()
    , MetaProvider_(std::move(metaProvider))
    , Logger_(logger)
    , Name_(std::move(name))
{
    auto& request = ProxyRequest_.Request;
    request.SetScheme(NAppHostHttp::THttpRequest::Http);
    request.SetPath(ToString(path));
    request.SetMethod(NAppHostHttp::THttpRequest::Get);

    {
        const auto childRTLogToken = CreateChildRTLogToken(logger, Name_);
        ProxyRequest_.RTLogToken.SetRTLogToken(childRTLogToken);
        AddHeader(NNetwork::HEADER_X_RTLOG_TOKEN, childRTLogToken);
    }
    AddHeader(NNetwork::HEADER_X_REQUEST_ID, MetaProvider_->GetRequestId());

    if (!MetaProvider_->GetClientIP().empty()) {
        // This header is important for users without Ya.Plus but who live in geo regions which are allowed to
        // listen to music for free. Without this header (and for such users), request from apphost located in MAN
        // (out of Russia datacenter) would be rejected by music backend. See HOLLYWOOD-233 for details.
        AddHeader(NNetwork::HEADER_X_FORWARDED_FOR, MetaProvider_->GetClientIP());
    }
}            

THttpProxyRequestBuilder::THttpProxyRequestBuilder(const TStringBuf path, const NScenarios::TRequestMeta& meta, TRTLogger& logger,
                                                   const TString name)
    : THttpProxyRequestBuilder(path, MakeAtomicShared<TRequestMetaProvider>(meta), logger, std::move(name))
{
}

TString THttpProxyRequestBuilder::CurrentDc_;

void THttpProxyRequestBuilder::SetCurrentDc(const TString& currentDc) {
    CurrentDc_ = currentDc;
}

THttpProxyRequestBuilder& THttpProxyRequestBuilder::SetBody(const TString& body, const TStringBuf contentType) {
    Y_ENSURE(!body.empty(), Name_);
    auto method = ProxyRequest_.Request.GetMethod();

    if (method != NAppHostHttp::THttpRequest::Post && method != NAppHostHttp::THttpRequest::Put) {
        ythrow yexception() << "Method " << NAppHostHttp::THttpRequest_EMethod_Name(method)
                            << " cannot have a body, request=" << Name_ << ", path=" << ProxyRequest_.Request.GetPath()
                            << " Check whether you forgot to call SetMethod with Post|Put first?";
    }

    ProxyRequest_.Request.SetContent(body);

    AddHeader(NNetwork::HEADER_CONTENT_TYPE, contentType);
    return *this;
}

THttpProxyRequestBuilder& THttpProxyRequestBuilder::SetMethod(NAppHostHttp::THttpRequest_EMethod method) {
    ProxyRequest_.Request.SetMethod(method);
    return *this;
}

THttpProxyRequestBuilder& THttpProxyRequestBuilder::SetScheme(NAppHostHttp::THttpRequest_EScheme scheme) {
    ProxyRequest_.Request.SetScheme(scheme);
    return *this;
}

THttpProxyRequestBuilder& THttpProxyRequestBuilder::AddHeader(const TStringBuf name, const TStringBuf value) {
    auto& header = *ProxyRequest_.Request.AddHeaders();
    header.SetName(TString{name});
    header.SetValue(TString{value});
    return *this;
}

THttpProxyRequestBuilder& THttpProxyRequestBuilder::AddHeaders(const THttpHeaders& headers) {
    for (const auto& header : headers) {
        AddHeader(header.Name(), header.Value());
    }
    return *this;
}

THttpProxyRequestBuilder& THttpProxyRequestBuilder::SetUseOAuth(const TString& oauthTokenPrefix) {
    const auto& oauthToken = MetaProvider_->GetOAuthToken();
    Y_ENSURE(!oauthToken.empty(), Name_);
    AddHeader(NNetwork::HEADER_AUTHORIZATION, oauthTokenPrefix + " " + oauthToken);
    return *this;
}

THttpProxyRequestBuilder& THttpProxyRequestBuilder::SetUseTVMUserTicket() {
    const auto& userTicket = MetaProvider_->GetUserTicket();
    Y_ENSURE(!userTicket.empty(), Name_);
    AddHeader(NNetwork::HEADER_X_YA_USER_TICKET, userTicket);
    return *this;
}

THttpProxyRequestBuilder& THttpProxyRequestBuilder::AddBalancingHint() {
    if (CurrentDc_.empty()) {
        LOG_INFO(Logger_) << "Cannot add balancing hint headers, because CurrentDc_ is empty. "
                          << "Did you forget to call SetCurrentDc function?";
        return *this;
    }

    if (IsIn(ALLOWED_DCS_FOR_BALANCING_HIT, CurrentDc_)) {
        AddHeader(TString(NNetwork::HEADER_X_BALANCER_DC_HINT), CurrentDc_);
        AddHeader(TString(NNetwork::HEADER_X_ALICE_INTERNAL_REQUEST), "yes");
    }
    return *this;
}   

THttpProxyRequest THttpProxyRequestBuilder::Build(const bool logVerbose) {
    LogRequest(Logger_, Name_, ProxyRequest_.Request, logVerbose);
    return ProxyRequest_;
}

THttpProxyRequest THttpProxyRequestBuilder::BuildAndMove(const bool logVerbose) && {
    LogRequest(Logger_, Name_, ProxyRequest_.Request, logVerbose);
    return std::move(ProxyRequest_);
}

// Replace without rtlog
NAppHostHttp::THttpRequest PrepareNoRtlogHttpRequest(const TStringBuf path,
                                     const NScenarios::TRequestMeta& meta,
                                     TRTLogger& logger,
                                     const TString& name,
                                     const TMaybe<TString> body,
                                     const TMaybe<NAppHostHttp::THttpRequest_EMethod> maybeMethod,
                                     const THttpHeaders& headers,
                                     bool useOAuth,
                                     bool useTVMUserTicket,
                                     const TString& oauthTokenPrefix)
{
    NAppHostHttp::THttpRequest request;
    request.SetScheme(NAppHostHttp::THttpRequest::Http);

    NAppHostHttp::THttpRequest_EMethod method;
    if (maybeMethod) {
        method = *maybeMethod;
    } else if (body) {
        method = NAppHostHttp::THttpRequest::Post;
    } else {
        method = NAppHostHttp::THttpRequest::Get;
    }
    request.SetMethod(method);
    if (body && (method == NAppHostHttp::THttpRequest::Post || method == NAppHostHttp::THttpRequest::Put)) {
        request.SetContent(body.GetRef());
    }
    request.SetPath(ToString(path));

    auto addHeader = [&request](const TStringBuf name, const TStringBuf value) {
        auto& header = *request.AddHeaders();
        header.SetName(TString{name});
        header.SetValue(TString{value});
    };

    for (const auto& header : headers) {
        addHeader(header.Name(), header.Value());
    }

    addHeader(NNetwork::HEADER_CONTENT_TYPE, NContentTypes::APPLICATION_JSON);
    addHeader(NNetwork::HEADER_X_REQUEST_ID, meta.GetRequestId());

    if (const auto& userTicket = meta.GetUserTicket(); useTVMUserTicket && !userTicket.empty()) {
        addHeader(NNetwork::HEADER_X_YA_USER_TICKET, userTicket);
    }

    if (const auto& oauthToken = meta.GetOAuthToken(); useOAuth && !oauthToken.empty()) {
        addHeader("Authorization", oauthTokenPrefix + " " + oauthToken);
    }

    LogRequest(logger, name, request);

    return request;
}

THttpProxyNoRtlogRequestBuilder::THttpProxyNoRtlogRequestBuilder(const TStringBuf path, TAtomicSharedPtr<IRequestMetaProvider> metaProvider, TRTLogger& logger,
                                                                 const TString name, bool randomizeRequestId)
    : ProxyRequest_()
    , MetaProvider_(std::move(metaProvider))
    , Logger_(logger)
    , Name_(std::move(name))
{
    auto& request = ProxyRequest_;
    request.SetScheme(NAppHostHttp::THttpRequest::Http);
    request.SetPath(ToString(path));
    request.SetMethod(NAppHostHttp::THttpRequest::Get);

    TString requestId = MetaProvider_->GetRequestId();
    if (randomizeRequestId) {
        requestId = TString::Join(requestId, "_", CreateGuidAsString());
    }
    AddHeader(NNetwork::HEADER_X_REQUEST_ID, requestId);

    if (!MetaProvider_->GetClientIP().empty()) {
        // This header is important for users without Ya.Plus but who live in geo regions which are allowed to
        // listen to music for free. Without this header (and for such users), request from apphost located in MAN
        // (out of Russia datacenter) would be rejected by music backend. See HOLLYWOOD-233 for details.
        AddHeader(NNetwork::HEADER_X_FORWARDED_FOR, MetaProvider_->GetClientIP());
    }
}            

THttpProxyNoRtlogRequestBuilder::THttpProxyNoRtlogRequestBuilder(const TStringBuf path, const NScenarios::TRequestMeta& meta, TRTLogger& logger,
                                                                 const TString name, bool randomizeRequestId)
    : THttpProxyNoRtlogRequestBuilder(path, MakeAtomicShared<TRequestMetaProvider>(meta), logger, std::move(name), randomizeRequestId)
{
}

TString THttpProxyNoRtlogRequestBuilder::CurrentDc_;

void THttpProxyNoRtlogRequestBuilder::SetCurrentDc(const TString& currentDc) {
    CurrentDc_ = currentDc;
}

THttpProxyNoRtlogRequestBuilder& THttpProxyNoRtlogRequestBuilder::SetBody(const TString& body, const TStringBuf contentType) {
    Y_ENSURE(!body.empty(), Name_);
    auto method = ProxyRequest_.GetMethod();

    if (method != NAppHostHttp::THttpRequest::Post && method != NAppHostHttp::THttpRequest::Put) {
        ythrow yexception() << "Method " << NAppHostHttp::THttpRequest_EMethod_Name(method)
                            << " cannot have a body, request=" << Name_ << ", path=" << ProxyRequest_.GetPath()
                            << " Check whether you forgot to call SetMethod with Post|Put first?";
    }

    ProxyRequest_.SetContent(body);

    AddHeader(NNetwork::HEADER_CONTENT_TYPE, contentType);
    return *this;
}

THttpProxyNoRtlogRequestBuilder& THttpProxyNoRtlogRequestBuilder::SetMethod(NAppHostHttp::THttpRequest_EMethod method) {
    ProxyRequest_.SetMethod(method);
    return *this;
}

THttpProxyNoRtlogRequestBuilder& THttpProxyNoRtlogRequestBuilder::SetScheme(NAppHostHttp::THttpRequest_EScheme scheme) {
    ProxyRequest_.SetScheme(scheme);
    return *this;
}

THttpProxyNoRtlogRequestBuilder& THttpProxyNoRtlogRequestBuilder::AddHeader(const TStringBuf name, const TStringBuf value) {
    auto& header = *ProxyRequest_.AddHeaders();
    header.SetName(TString{name});
    header.SetValue(TString{value});
    return *this;
}

THttpProxyNoRtlogRequestBuilder& THttpProxyNoRtlogRequestBuilder::AddHeaders(const THttpHeaders& headers) {
    for (const auto& header : headers) {
        AddHeader(header.Name(), header.Value());
    }
    return *this;
}

THttpProxyNoRtlogRequestBuilder& THttpProxyNoRtlogRequestBuilder::SetUseOAuth(const TString& oauthTokenPrefix) {
    const auto& oauthToken = MetaProvider_->GetOAuthToken();
    Y_ENSURE(!oauthToken.empty(), Name_);
    AddHeader(NNetwork::HEADER_AUTHORIZATION, oauthTokenPrefix + " " + oauthToken);
    return *this;
}

THttpProxyNoRtlogRequestBuilder& THttpProxyNoRtlogRequestBuilder::SetUseTVMUserTicket() {
    const auto& userTicket = MetaProvider_->GetUserTicket();
    Y_ENSURE(!userTicket.empty(), Name_);
    AddHeader(NNetwork::HEADER_X_YA_USER_TICKET, userTicket);
    return *this;
}

THttpProxyNoRtlogRequestBuilder& THttpProxyNoRtlogRequestBuilder::AddBalancingHint() {
    if (CurrentDc_.empty()) {
        LOG_INFO(Logger_) << "Cannot add balancing hint headers, because CurrentDc_ is empty. "
                          << "Did you forget to call SetCurrentDc function?";
        return *this;
    }

    if (IsIn(ALLOWED_DCS_FOR_BALANCING_HIT, CurrentDc_)) {
        AddHeader(TString(NNetwork::HEADER_X_BALANCER_DC_HINT), CurrentDc_);
        AddHeader(TString(NNetwork::HEADER_X_ALICE_INTERNAL_REQUEST), "yes");
    }
    return *this;
}   

NAppHostHttp::THttpRequest THttpProxyNoRtlogRequestBuilder::Build(const bool logVerbose) {
    LogRequest(Logger_, Name_, ProxyRequest_, logVerbose);
    return ProxyRequest_;
}

NAppHostHttp::THttpRequest THttpProxyNoRtlogRequestBuilder::BuildAndMove(const bool logVerbose) && {
    LogRequest(Logger_, Name_, ProxyRequest_, logVerbose);
    return std::move(ProxyRequest_);
}

} // namespace NAlice::NHollywood
