#pragma once

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/fetcher/request.h>

#include <library/cpp/cgiparam/cgiparam.h>

namespace NAlice::NTestingHelpers {

class TFakeHandle : public NHttpFetcher::THandle {
public:
    TFakeHandle(int code, TStringBuf body)
        : Code(code)
        , Body(body) {
    }

    TFakeHandle(int code, TStringBuf body, const THttpHeaders& headers)
        : Code(code)
        , Body(body)
        , Headers(headers) {
    }

    NHttpFetcher::TResponse::TRef Wait(TInstant /* deadline */) override {
        return MakeIntrusive<NHttpFetcher::TResponse>(Code, Body, TDuration{}, TStringBuf{} /* errMsg */,
                                                      THttpHeaders{Headers});
    }

private:
    int Code;
    TString Body;
    THttpHeaders Headers;
};

class TFakeRequest : public NHttpFetcher::TRequest {
public:
    TFakeRequest() = default;

    explicit TFakeRequest(TString data, int statusCode = HttpCodes::HTTP_OK, THttpHeaders&& responseHeaders = {})
        : Data(std::move(data))
        , StatusCode(statusCode)
        , ResponseHeaders(std::move(responseHeaders))
    {
    }

    NHttpFetcher::THandle::TRef Fetch() override {
        return MakeIntrusive<TFakeHandle>(StatusCode, Data, ResponseHeaders);
    }

    TRequest& SetProxy(const TString& /* proxy */) override {
        return *this;
    }

    TRequest& AddHeader(TStringBuf name, TStringBuf value) override {
        Headers.push_back(THeaderValue{.Name = TString{name}, .Value = TString{value}});
        return *this;
    }

    TVector<TString> GetHeaders() const override;

    bool HasHeader(TStringBuf name) const override {
        return FindIfPtr(Headers, [name](const auto& header) { return header.Name == name; });
    }

    TRequest& SetMethod(TStringBuf /* method */) override {
        return *this;
    }

    TRequest& SetBody(TStringBuf /* body */, TStringBuf /* method */) override {
        return *this;
    }

    void SetPath(TStringBuf /* path */) override {
    }
    const TString& GetBody() const override {
        return Dummy;
    }

    TString GetMethod() const override {
        return "GET";
    }

    TString Url() const override;

    TString Data;
    int StatusCode = HttpCodes::HTTP_OK;

private:
    struct THeaderValue {
        TString Name;
        TString Value;
    };

    const TString Dummy;
    THttpHeaders ResponseHeaders;
    TVector<THeaderValue> Headers;

};

struct TFakeMultiRequest : public NHttpFetcher::IMultiRequest {
public:
    explicit TFakeMultiRequest(TString data)
        : Request(std::move(data)) {
    }

    THolder<NHttpFetcher::TRequest> AddRequest(const NUri::TUri& /* uri */,
                                               const NHttpFetcher::TRequestOptions& /* options */) override {
        return MakeHolder<TFakeRequest>(Request);
    }

    void WaitAll(TInstant /* deadline */) override {
    }

private:
    TFakeRequest Request;
};

class TFakeRequestBuilder final : public NHttpFetcher::IRequestBuilder {
public:
    TCgiParameters Cgi;
    TMaybe<TString> Proxy;
    TMaybe<TString> Method;
    TMaybe<TString> Body;
    TMaybe<TString> ContentType;
    TMaybe<TString> RequestLabel;
    THttpHeaders Headers;

public:
    NHttpFetcher::IRequestBuilder& AddCgiParams(const TCgiParameters& cgi) override {
        for (const auto& kv : cgi) {
            Cgi.InsertUnescaped(kv.first, kv.second);
        }
        return *this;
    }

    NHttpFetcher::IRequestBuilder& AddCgiParam(TStringBuf key, TStringBuf value) override {
        Cgi.InsertUnescaped(key, value);
        return *this;
    }

    NHttpFetcher::IRequestBuilder& ReplaceCgiParams(const TCgiParameters& cgi) override {
        for (const auto& kv : cgi) {
            Cgi.ReplaceUnescaped(kv.first, kv.second);
        }
        return *this;
    }

    NHttpFetcher::IRequestBuilder& ReplaceCgiParam(TStringBuf key, TStringBuf value) override {
        Cgi.ReplaceUnescaped(key, value);
        return *this;
    }

    NHttpFetcher::IRequestBuilder& SetProxy(const TString& proxy) override {
        Proxy = proxy;
        return *this;
    }

    NHttpFetcher::IRequestBuilder& AddHeader(TStringBuf key, TStringBuf value) override {
        Headers.AddHeader(TString{key}, value);
        return *this;
    }

    NHttpFetcher::IRequestBuilder& SetMethod(TStringBuf method) override {
        Method = method;
        return *this;
    }

    NHttpFetcher::IRequestBuilder& SetBody(TStringBuf body, TStringBuf method = TStringBuf("")) override {
        if (!method.Empty()) {
            Method = method;
        }
        Body = body;
        return *this;
    }

    NHttpFetcher::IRequestBuilder& SetContentType(TStringBuf value) override {
        ContentType = value;
        return *this;
    }

    IRequestBuilder& SetRequestLabel(const TString& label) override {
        RequestLabel = label;
        return *this;
    }

    bool HasHeader(TStringBuf header) const override {
        return Headers.HasHeader(header);
    }

    bool HasHeader(TStringBuf header, TStringBuf value) const {
        const auto* headerValue = Headers.FindHeader(header);
        return headerValue && headerValue->Value() == value;
    }

    TString DebugString() const;
};

} // namespace NAlice::NTestingHelpers
