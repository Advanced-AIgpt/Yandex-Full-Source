#include "http_utils.h"

#include <alice/library/json/json.h>

namespace NAlice::NHollywood::NFood {

THttpHeaders MakeHeaders() {
    THttpHeaders headers;
    headers.AddHeader("x-app-version", "15.12.0");
    headers.AddHeader("x-platform", "desktop_web");
    return headers;
}

THttpHeaders MakeHeaders(TStringBuf phpsessid) {
    THttpHeaders headers = MakeHeaders();
    headers.AddHeader("cookie", TString{"PHPSESSID="} + phpsessid);
    return headers;
}

THttpHeaders MakeHeadersWithTaxiUid(TStringBuf taxiUid) {
    THttpHeaders headers = MakeHeaders();
    headers.AddHeader("X-YaTaxi-UserId", taxiUid);
    return headers;
}

THttpProxyRequest PrepareHttpRequest(
    const TScenarioHandleContext& ctx,
    const TString& path,
    const THttpHeaders& headers,
    const TMaybe<TString>& body,
    const bool useOAuth
) {
    THttpProxyRequest req = ::NAlice::NHollywood::PrepareHttpRequest(
        /* path= */ path,
        /* meta= */ ctx.RequestMeta,
        /* logger= */ ctx.Ctx.Logger(),
        /* name= */ "",
        /* body= */ body,
        /* method= */ body.Defined() ? NAppHostHttp::THttpRequest::Post : NAppHostHttp::THttpRequest::Get,
        /* headers= */ headers,
        /* useOAuth= */ useOAuth,
        /* useTVMUserTicket= */ true,
        /* oauthTokenPrefix= */ "Bearer"
    );
    return req;
}

THttpProxyRequest PrepareHttpRequest(
    const TScenarioHandleContext& ctx,
    const TString& path,
    const THttpHeaders& headers,
    const NJson::TJsonValue& json,
    const bool useOAuth
) {
    const TMaybe<TString> body = json.IsDefined() ? JsonToString(json) : TString{""};
    return PrepareHttpRequest(ctx, path, headers, body, useOAuth);
}

TMaybe<TJsonHttpResponse> RetireHttpResponseJsonExtendedMaybe(
    const TScenarioHandleContext& ctx,
    TStringBuf responseKey,
    TStringBuf tokenKey,
    bool logBody
) {
    Y_UNUSED(tokenKey);
    const TBassRequestRTLogToken requestRTLogToken;

    auto maybeResponse = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(ctx.ServiceCtx, responseKey);
    THttpHeaders headers;
    if (maybeResponse.Defined()) {
        for (const auto& header : maybeResponse->GetHeaders()) {
            headers.AddHeader(header.GetName(), header.GetValue());
        }
    }
    auto maybeJson = RetireJsonResponseMaybe(std::move(maybeResponse), requestRTLogToken,
        ctx.Ctx.Logger(), logBody, /* throwOnFailure = */ false);
    if (!maybeJson.Defined()) {
        LOG_WARN(ctx.Ctx.Logger()) << "No json response";
        return Nothing();
    }
    return TJsonHttpResponse{
        .HttpCode = maybeResponse->GetStatusCode(),
        .Headers = std::move(headers),
        .Body = std::move(*maybeJson)
    };
}

TJsonHttpResponse RetireHttpResponseJsonExtended(
    const TScenarioHandleContext& ctx,
    TStringBuf responseKey,
    TStringBuf tokenKey,
    bool logBody
) {
    Y_UNUSED(tokenKey);
    const TBassRequestRTLogToken requestRTLogToken;
    auto maybeResponse = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(ctx.ServiceCtx, responseKey);
    TJsonHttpResponse result;
    if (maybeResponse.Defined()) {
        for (const auto& header : maybeResponse->GetHeaders()) {
            result.Headers.AddHeader(header.GetName(), header.GetValue());
        }
    }
    result.Body = *RetireJsonResponseMaybe(std::move(maybeResponse), requestRTLogToken,
        ctx.Ctx.Logger(), logBody, /* throwOnFailure = */ true);
    return result;
}

TString GetTaxiUid(const THttpHeaders& headers) {
    for (const auto& header : headers) {
        TString headerName = header.Name();
        headerName.to_lower();
        if (headerName == "x-yataxi-userid") {
            return header.Value();
        }
    }
    return "";
}

}  // namespace NAlice::NHollywood::NFood
