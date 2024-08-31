#include "proxy_request_builder.h"

namespace NAlice::NVideoCommon {

void TProxyRequestBuilder::AddCgiParam(const TStringBuf key, const TStringBuf value) {
    Cgi.InsertEscaped(key, value);
}

void TProxyRequestBuilder::SetEndpoint(const TString& endpoint) {
    Endpoint = endpoint;
}

void TProxyRequestBuilder::SetScheme(const NAppHostHttp::THttpRequest_EScheme scheme) {
    Scheme = scheme;
}

void TProxyRequestBuilder::AddHeader(const TStringBuf key, const TStringBuf value) {
    Headers.emplace_back(key, value);
}

NHollywood::THttpProxyRequest TProxyRequestBuilder::Build() const {
    TString path = TString::Join(Endpoint.Defined() ? ("/" + Endpoint.GetRef()) : "", "?", Cgi.Print());

    bool useOauth = !DisableOauth;
    auto requestStruct = NHollywood::PrepareHttpRequest(path, RequestMeta, Logger, ""
        , Nothing(), Nothing(), Default<THttpHeaders>(), useOauth, true, "OAuth");

    auto& request = requestStruct.Request;
    for (const auto& [key, value] : Headers) {
        NAppHostHttp::THeader* header = request.AddHeaders();
        header->SetName(key);
        header->SetValue(value);
    }

    if (Scheme.Defined()) {
        request.SetScheme(Scheme.GetRef());
    }

    return requestStruct;
}

} // namespace NAlice::NVideoCommon
