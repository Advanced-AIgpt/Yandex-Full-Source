#pragma once

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/http/io/headers.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>

namespace NAlice::NHollywood::NFood {

struct TJsonHttpResponse {
    ui32 HttpCode;
    THttpHeaders Headers;
    NJson::TJsonValue Body;
};

THttpHeaders MakeHeaders();
THttpHeaders MakeHeaders(TStringBuf phpsessid);
THttpHeaders MakeHeadersWithTaxiUid(TStringBuf taxiUid);

THttpProxyRequest PrepareHttpRequest(
    const TScenarioHandleContext& ctx,
    const TString& path,
    const THttpHeaders& headers,
    const TMaybe<TString>& body = Nothing(),
    const bool useOAuth = false
);

THttpProxyRequest PrepareHttpRequest(
    const TScenarioHandleContext& ctx,
    const TString& path,
    const THttpHeaders& headers,
    const NJson::TJsonValue& json,
    const bool useOAuth = false
);

TMaybe<TJsonHttpResponse> RetireHttpResponseJsonExtendedMaybe(
    const TScenarioHandleContext& ctx,
    TStringBuf responseKey = PROXY_RESPONSE_KEY_DEFAULT,
    TStringBuf tokenKey = PROXY_RTLOG_TOKEN_KEY_DEFAULT,
    bool logBody = false
);

TJsonHttpResponse RetireHttpResponseJsonExtended(
    const TScenarioHandleContext& ctx,
    TStringBuf responseKey = PROXY_RESPONSE_KEY_DEFAULT,
    TStringBuf tokenKey = PROXY_RTLOG_TOKEN_KEY_DEFAULT,
    bool logBody = false
);

template <typename TProto>
inline TProto GetLastProtoOrThrow(const NAppHost::IServiceContext& ctx, const TStringBuf type) {
    const auto& items = ctx.GetProtobufItemRefs(type);
    Y_ENSURE(!items.empty(), "No proto item of type " << type);
    return ParseProto<TProto>(items.back().Raw());
}

TString GetTaxiUid(const THttpHeaders& headers);

} // namespace NAlice::NHollywood::NFood
