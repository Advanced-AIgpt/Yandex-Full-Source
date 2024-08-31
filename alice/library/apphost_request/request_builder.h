#pragma once

#include <alice/library/network/request_builder.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/generic/strbuf.h>
#include <util/string/ascii.h>
#include <util/string/builder.h>

namespace NAlice::NAppHostRequest {

class TAppHostHttpProxyRequestBuilder : public NNetwork::IRequestBuilder {
public:
    using EMethod = NAppHostHttp::THttpRequest::EMethod;
    using EScheme = NAppHostHttp::THttpRequest::EScheme;

public:
    TAppHostHttpProxyRequestBuilder();

    // Modify Request.Path and Cgi_ fields
    NAppHostHttp::THttpRequest& CreateRequest();

    TAppHostHttpProxyRequestBuilder& SetScheme(EScheme scheme);
    TAppHostHttpProxyRequestBuilder& SetMethod(EMethod method);
    TAppHostHttpProxyRequestBuilder& SetPath(const TStringBuf path);

    // Overrides NHttpFetcher::IRequestBuilder.
    IRequestBuilder& AddCgiParams(const TCgiParameters& cgi) override final;
    IRequestBuilder& AddCgiParam(TStringBuf name, TStringBuf value) override final;
    IRequestBuilder& ReplaceCgiParams(const TCgiParameters& cgi) override  final;
    IRequestBuilder& ReplaceCgiParam(TStringBuf key, TStringBuf value) override final;

    // TODO (petrk) Implement it when understand how.
    IRequestBuilder& SetProxy(const TString& /* proxy */) override final {
        return *this;
    }

    IRequestBuilder& AddHeader(TStringBuf name, TStringBuf value) override final;
    IRequestBuilder& SetMethod(TStringBuf methodStr) override final;
    IRequestBuilder& SetBody(TStringBuf body, TStringBuf method) override final;

    IRequestBuilder& SetContentType(TStringBuf value) override final;

    // FIXME (petrk) Implement it.
    IRequestBuilder& SetRequestLabel(const TString& /* label */) override final {
        return *this;
    }

    bool HasHeader(TStringBuf header) const override final;

private:
    NAppHostHttp::THttpRequest Request_;
    TCgiParameters Cgi_;
    bool RequestConstructed_ = false;
};

} // namespace NAlice::NAppHostRequest
