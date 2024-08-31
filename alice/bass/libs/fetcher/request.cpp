#include "request.h"

#include "util.h"

#include <alice/library/network/common.h>
#include <alice/library/network/headers.h>

#include <util/generic/serialized_enum.h>
#include <util/string/split.h>
#include <util/system/shellcommand.h>

namespace NHttpFetcher {

using namespace NAlice::NNetwork;

namespace {

bool IsHttps(TStringBuf url) {
    const TStringBuf scheme = url.NextTok("://");
    return TStringBuf("fulls") == scheme
           || TStringBuf("https") == scheme
           || TStringBuf("posts") == scheme
           || TStringBuf("gets") == scheme;
}

} // namespace

// TProxySettings -------------------------------------------------------------
// static
TIntrusivePtr<TProxySettings> TProxySettings::CreateFromHeader(TStringBuf header) {
    TStringBuf hostPort;
    TStringBuf options;
    size_t hostPortFound = 0;
    auto onEachChar = [&hostPortFound](char ch) {
        if (ch != ':') {
            return false;
        }
        // Search for second ':' because want to separate `host:port` from `options`.
        return ++hostPortFound == 2;
    };
    StringSplitter(header).SplitByFunc(onEachChar).TryCollectInto(&hostPort, &options);

    if (hostPort.Empty()) {
        return {};
    }

    EProxyMode mode = EProxyMode::BehindBalancer; // Default mode is behind balancer.
    if (!options.Empty()) {
        if (!TryFromString(options, mode)) {
            return {};
        }
    }

    return MakeIntrusive<TProxySettings>(ToString(hostPort), mode);
}

const TString& TProxySettings::GetHostPort(TStringBuf url) const {
    if (IsHttps(url) && !Https.empty()) {
        return Https;
    }
    return Http;
}

TString TProxySettings::MakeUriWithCorrectScheme(TStringBuf url) const {
    if (IsHttps(url) && !Https.empty()) {
        return TStringBuilder() << TStringBuf("https://") << Https;
    }
    return TStringBuilder() << TStringBuf("http://") << Http;
}

void TProxySettings::PrepareRequest(const NUri::TUri& uri, const TCgiParameters& cgi, TString& url, TString& headers, TMaybe<TString>& overrideHostPort) const {
    if (Proxy) {
        Proxy->PrepareRequest(uri, cgi, url, headers, overrideHostPort);
    }

    if (!Headers.Empty()) {
        AddHeadersToString(Headers, headers);
    }

    auto proxyMode = ProxyMode;
    if (IsLocalhost(uri.GetHost())) {
        return;
    }

    switch (proxyMode) {
        case TProxySettings::EProxyMode::BehindBalancer:
        {
            TString origUrl = UrlWithCgiParams(uri, cgi);
            AddHeaderToString(HEADER_X_HOST, origUrl, headers);

            NUri::TUri proxyUrl;
            proxyUrl.Parse(MakeUriWithCorrectScheme(origUrl), NUri::TUri::FeaturesRecommended);
            url = UrlWithCgiParams(proxyUrl, cgi);
            break;
        }

        case TProxySettings::EProxyMode::Normal:
        {
            if (Proxy) {
                AddHeaderToString(HEADER_X_YANDEX_VIA_PROXY, GetHostPort(uri.PrintS()), headers);
            } else {
                AddHeaderToString(HEADER_X_HOST, UrlWithCgiParams(uri, cgi), headers);
                overrideHostPort = GetHostPort(uri.PrintS());
            }
            break;
        }
    }
}

// TResponse ------------------------------------------------------------------
TResponse::TResponse(EResult result, TDuration duration, TStringBuf errMsg, TSystemErrorCode systemErrorCode)
    : Result(result)
    , Code(0)
    , Duration(duration)
    , ErrMsg(errMsg)
    , SystemErrorCode(systemErrorCode)
{
}

TResponse::TResponse(THttpCode code, TString body, TDuration duration, TStringBuf errMsg, THttpHeaders&& headers)
    : Result((IsUserError(code) || IsServerError(code)) ? EResult::HttpError : EResult::Ok)
    , Code(code)
    , Data(body)
    , Headers(std::move(headers))
    , Duration(duration)
    , ErrMsg(errMsg)
{
}

const TString& TResponse::GetErrorText() const {
    return ErrMsg;
}

TResponse::TSystemErrorCode TResponse::GetSystemErrorCode() const {
    return SystemErrorCode;
}

bool TResponse::IsError() const {
    return Result != EResult::Ok;
}

bool TResponse::IsHttpOk() const {
    return !IsError() && Code == HttpCodes::HTTP_OK;
}

bool TResponse::IsTimeout() const {
    return Result == EResult::Timeout;
}

THttpResponse TResponse::ToHttpResponse() const {
    THttpResponse response;
    if (const auto code = ToHttpCodes(Code); code.Defined())
        response.SetHttpCode(*code);
    response.AddMultipleHeaders(Headers);
    response.SetContent(Data);
    return response;
}

TRequest& TRequest::AddCgiParam(TStringBuf key, TStringBuf value) {
    Cgi.InsertUnescaped(key, value);
    return *this;
}

TRequest& TRequest::AddCgiParams(const TCgiParameters& cgi) {
    for (const auto& kv : cgi) {
        Cgi.InsertUnescaped(kv.first, kv.second);
    }
    return *this;
}

TString TRequest::AsCurl(bool withBody) const {
    const TString url{Url()};
    const TVector<TString> headers{GetHeaders()};
    TStringBuilder request;
    request << "curl -v '" << Url() << '\'';
    request << " -X " << GetMethod();
    for (const TString& header : headers) {
        request << " -H";
        ShellQuoteArgSp(request, header);
    }
    if (withBody) {
        if (TString body = GetBody(); !body.empty()) {
            request << " -d";
            ShellQuoteArgSp(request, body);
        }
    }
    return request;
}

TRequest& TRequest::ReplaceCgiParam(TStringBuf key, TStringBuf value) {
    Cgi.ReplaceUnescaped(key, value);
    return *this;
}

TRequest& TRequest::ReplaceCgiParams(const TCgiParameters& cgi) {
    for (const auto& kv : cgi) {
        Cgi.ReplaceUnescaped(kv.first, kv.second);
    }
    return *this;
}

TRequest& TRequest::SetContentType(TStringBuf value) {
    return AddHeader(TStringBuf("Content-type"), value);
}

TRequest& TRequest::SetRequestLabel(const TString& label) {
    RequestLabel = label;
    return *this;
}

} // NHttpFetcher
