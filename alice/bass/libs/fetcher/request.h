#pragma once

#include "fwd.h"

#include <alice/library/network/request_builder.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/http/io/headers.h>
#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/http/server/response.h>
#include <library/cpp/uri/uri.h>

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>

namespace NHttpFetcher {

class TResponse: public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TResponse>;
    using TConstRef = TIntrusiveConstPtr<TResponse>;
    using TSystemErrorCode = i32;

public:
    using THttpCode = int;
    enum class EResult : ui8 {
        Ok,                    /// fetched successfully and server returns 200
        Timeout,               /// fetch timeout (no Code and Body is set)
        EmptyResponse,         /// received empty response (no Code and Body set)
        ParsingError,          /// http parsing response error (no Code and Body set)
        HttpError,             /// successfully received response but http != 200 (Code and Body are set)
        DataError,             /// response data level error (can be returned from TCustomResponseCallback)
        NetworkResolutionError /// network resolution error
    };

    EResult Result;
    THttpCode Code;
    TString Data;
    THttpHeaders Headers;

    /// Time taken to get this response
    TDuration Duration;

    TString RTLogToken;

public:
    TResponse(EResult result, TDuration duration, TStringBuf errMsg, TSystemErrorCode systemErrorCode);
    TResponse(THttpCode code, TString body, TDuration duration, TStringBuf errMsg, THttpHeaders&& headers);

    bool IsError() const;
    bool IsHttpOk() const;
    bool IsTimeout() const;
    const TString& GetErrorText() const;
    TSystemErrorCode GetSystemErrorCode() const;

    THttpResponse ToHttpResponse() const;

private:
    TString ErrMsg;
    TSystemErrorCode SystemErrorCode = 0;
};

class THandle : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<THandle>;

public:
    /** Wait response till deadline.
     * If response hasn't received, cancel request!!!
     * XXX the behaviour has to be changed. It has to wait but not cancel
     * request if deadline happened, just return nullptr (don't forget to introduce Cancel() method.
     */
    virtual TResponse::TRef Wait(TInstant deadline) = 0;

    TResponse::TRef Wait() {
        return Wait(TInstant::Max());
    }

    TResponse::TRef WaitFor(TDuration duration) {
        return Wait(TInstant::Now() + duration);
    }

    TResponse::TRef WaitUntil(TInstant deadline) {
        return Wait(deadline);
    }

public:
    TResponse::TRef Response;
};

class TProxySettings : public TThrRefBase {
public:
    enum class EProxyMode {
        /// Add header x-host with original url + cgi. Replace main url with proxy.
        BehindBalancer /* "behind_balancer" */,
        /// Add header x-host with original url + cgi. Replace connection address with full://proxyhost:proxyport.
        /// Use absolute url in first line: GET http://ya.ru/?text=hello HTTP/1.1
        Normal /* "normal" */,
    };

public:
    TProxySettings(const TString& proxy, EProxyMode proxyMode)
        : TProxySettings(proxy, THttpHeaders{}, proxyMode)
    {
    }

    template <typename T>
    TProxySettings(const TString& proxy, T&& headers, EProxyMode proxyMode)
        : Http(proxy)
        , Https(proxy)
        , Headers(std::forward<T>(headers))
        , ProxyMode(proxyMode)
    {
    }

    /** Try to create settings from header.
     * Valid values:
     *  'host:port' - mode will be BehindBalancer
     *  'host:port:behind_balancer' - mode will be BehindBalancer
     *  'host:port:normal' - mode will be Normal
     */
    static TIntrusivePtr<TProxySettings> CreateFromHeader(TStringBuf header);

    void ViaProxy(TProxySettingsPtr proxy) {
        Proxy = proxy;
    }

    const TString& GetHostPort(TStringBuf url) const;

    // get 'http://' + hostport or 'https://' + hostport according to `url` scheme
    TString MakeUriWithCorrectScheme(TStringBuf url) const;

    const THttpHeaders& GetHeaders() const {
        return Headers;
    }

    void AddHeader(const TString& name, const TString& value) {
        Headers.AddHeader(name, value);
    }

    void PrepareRequest(const NUri::TUri& uri, const TCgiParameters& cgi, TString& url, TString& headers, TMaybe<TString>& overrideHostPort) const;

    template <typename T>
    static TString CreateHeaderViaProxy(TStringBuf host, T&& port) {
        return TString::Join(host, ':', ToString(port));
    }

private:
    TString Http;
    TString Https;
    THttpHeaders Headers;
    EProxyMode ProxyMode;
    TProxySettingsPtr Proxy;
};

/** Interface for different requests
 */
class TRequest {
public:
    virtual ~TRequest() = default;

    /** Run this request!
     */
    virtual THandle::TRef Fetch() = 0;

    virtual TRequest& AddCgiParams(const TCgiParameters& cgi);
    virtual TRequest& AddCgiParam(TStringBuf key, TStringBuf value);

    virtual TRequest& ReplaceCgiParams(const TCgiParameters& cgi);
    virtual TRequest& ReplaceCgiParam(TStringBuf key, TStringBuf value);

    virtual TRequest& SetProxy(const TString& proxy) = 0;

    virtual void SetPath(TStringBuf path) = 0;

    /** Add header to request
     */
    virtual TRequest& AddHeader(TStringBuf key, TStringBuf value) = 0;

    TRequest& AddHeader(const THttpInputHeader& header) {
        return AddHeader(header.Name(), header.Value());
    }

    void AddHeaders(const THttpHeaders& headers) {
        for (const auto& header : headers)
            AddHeader(header);
    }

    /** Get headers of request
     */
    virtual TVector<TString> GetHeaders() const = 0;

    /** Check if header is present
     */
    virtual bool HasHeader(TStringBuf name) const = 0;

    /** Get request string as curl command
     */
    virtual TString AsCurl(bool withBody = true) const;

    /** Set request method, POST, GET, etc...
     */
    virtual TRequest& SetMethod(TStringBuf method) = 0;

    /** Set request body and method (if necessary)
     */
    virtual TRequest& SetBody(TStringBuf body, TStringBuf method = TStringBuf("")) = 0;

    /** Gey request body
     */
    virtual const TString& GetBody() const = 0;

    /** Get request method (like GET, POST, etc...)
     */
    virtual TString GetMethod() const = 0;

    /** An easy way to set Content-type header
     */
    virtual TRequest& SetContentType(TStringBuf value);

    /** Request label for rtlog
     */
    virtual TRequest& SetRequestLabel(const TString& label);

    virtual TString Url() const = 0;

protected:
    TCgiParameters Cgi;
    TString RequestLabel;
};

/// @deprecated
/// Please use full name NAlice::NNetworkd::IRequestBuilder instaed of NHttpFetcher::IRequestBuilder.
/// This using is for backward compatibility and will be removed!
using NAlice::NNetwork::IRequestBuilder;

class TRequestBuilder final : public NAlice::NNetwork::IRequestBuilder {
public:
    explicit TRequestBuilder(TRequest& request)
        : Request(request) {
    }

    IRequestBuilder& AddCgiParams(const TCgiParameters& cgi) override {
        Request.AddCgiParams(cgi);
        return *this;
    }

    IRequestBuilder& AddCgiParam(TStringBuf key, TStringBuf value) override {
        Request.AddCgiParam(key, value);
        return *this;
    }

    IRequestBuilder& ReplaceCgiParams(const TCgiParameters& cgi) override {
        Request.ReplaceCgiParams(cgi);
        return *this;
    }

    IRequestBuilder& ReplaceCgiParam(TStringBuf key, TStringBuf value) override {
        Request.ReplaceCgiParam(key, value);
        return *this;
    }

    IRequestBuilder& SetProxy(const TString& proxy) override {
        Request.SetProxy(proxy);
        return *this;
    }

    IRequestBuilder& AddHeader(TStringBuf key, TStringBuf value) override {
        Request.AddHeader(key, value);
        return *this;
    }

    IRequestBuilder& SetMethod(TStringBuf method) override {
        Request.SetMethod(method);
        return *this;
    }

    IRequestBuilder& SetBody(TStringBuf body, TStringBuf method = TStringBuf("")) override {
        Request.SetBody(body, method);
        return *this;
    }

    IRequestBuilder& SetContentType(TStringBuf value) override {
        Request.SetContentType(value);
        return *this;
    }

    IRequestBuilder& SetRequestLabel(const TString& label) override {
        Request.SetRequestLabel(label);
        return *this;
    }

    bool HasHeader(TStringBuf name) const override {
        return Request.HasHeader(name);
    }

private:
    TRequest& Request;
};

} // namespace NHttpFetcher
