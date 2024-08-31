#include "request_builder.h"

#include <alice/library/network/headers.h>

#include <util/generic/algorithm.h>
#include <util/string/ascii.h>
#include <util/string/cast.h>

namespace NAlice::NAppHostRequest {

TAppHostHttpProxyRequestBuilder::TAppHostHttpProxyRequestBuilder() {
    Request_.SetScheme(EScheme::THttpRequest_EScheme_Http);
    Request_.SetMethod(EMethod::THttpRequest_EMethod_Get);
}

NAppHostHttp::THttpRequest& TAppHostHttpProxyRequestBuilder::CreateRequest() {
    if (RequestConstructed_) {
        ythrow yexception() << "Request has already been constructed";
    }
    if (!Cgi_.empty()) {
        Request_.SetPath(TStringBuilder{} << Request_.GetPath() << '?' << Cgi_.Print());
        Cgi_.Flush();
    }
    RequestConstructed_ = true;

    return Request_;
}

TAppHostHttpProxyRequestBuilder& TAppHostHttpProxyRequestBuilder::SetScheme(EScheme scheme) {
    Request_.SetScheme(scheme);
    return *this;
}

TAppHostHttpProxyRequestBuilder& TAppHostHttpProxyRequestBuilder::SetMethod(EMethod method) {
    Request_.SetMethod(method);
    return *this;
}

TAppHostHttpProxyRequestBuilder& TAppHostHttpProxyRequestBuilder::SetPath(const TStringBuf path) {
    Request_.SetPath(TString{path});
    return *this;
}

NNetwork::IRequestBuilder& TAppHostHttpProxyRequestBuilder::AddCgiParams(const TCgiParameters& cgi) {
    for (const auto& kv : cgi) {
        Cgi_.InsertUnescaped(kv.first, kv.second);
    }
    return *this;
}

NNetwork::IRequestBuilder& TAppHostHttpProxyRequestBuilder::AddCgiParam(TStringBuf name, TStringBuf value) {
    Cgi_.InsertUnescaped(name, value);
    return *this;
}

NNetwork::IRequestBuilder& TAppHostHttpProxyRequestBuilder::ReplaceCgiParams(const TCgiParameters& cgi) {
    for (const auto& kv : cgi) {
        Cgi_.ReplaceUnescaped(kv.first, kv.second);
    }
    return *this;
}

NNetwork::IRequestBuilder& TAppHostHttpProxyRequestBuilder::ReplaceCgiParam(TStringBuf key, TStringBuf value) {
    Cgi_.ReplaceUnescaped(key, value);
    return *this;
}

NNetwork::IRequestBuilder& TAppHostHttpProxyRequestBuilder::AddHeader(TStringBuf name, TStringBuf value) {
    auto* header = Request_.AddHeaders();
    header->SetName(TString{name});
    header->SetValue(TString{value});
    return *this;
}

NNetwork::IRequestBuilder& TAppHostHttpProxyRequestBuilder::SetMethod(TStringBuf methodStr) {
    auto is = [methodStr](TStringBuf method) -> bool {
        return AsciiCompareIgnoreCase(method, methodStr) == 0;
    };

    NAppHostHttp::THttpRequest::EMethod method;
    if (is(TStringBuf("get"))) {
        method = NAppHostHttp::THttpRequest::EMethod::THttpRequest_EMethod_Get;
    } else if (is(TStringBuf("post"))) {
        method = NAppHostHttp::THttpRequest::EMethod::THttpRequest_EMethod_Post;
    } else if (is(TStringBuf("put"))) {
        method = NAppHostHttp::THttpRequest::EMethod::THttpRequest_EMethod_Put;
    } else if (is(TStringBuf("delete"))) {
        method = NAppHostHttp::THttpRequest::EMethod::THttpRequest_EMethod_Delete;
    } else {
        ythrow yexception() << "method '" << methodStr << "' is not supported for apphost http request";
    }

    return SetMethod(method);
}

NNetwork::IRequestBuilder& TAppHostHttpProxyRequestBuilder::SetBody(TStringBuf body, TStringBuf method) {
    Request_.SetContent(TString{body});
    return SetMethod(method);
}

NNetwork::IRequestBuilder& TAppHostHttpProxyRequestBuilder::SetContentType(TStringBuf value) {
    auto& headers = *Request_.MutableHeaders();
    auto it = std::find_if(headers.begin(), headers.end(),
                           [](auto& h) { return 0 == AsciiCompareIgnoreCase(NNetwork::HEADER_CONTENT_TYPE, h.GetName()); });
    if (it != headers.end()) {
        it->SetValue(ToString(value));
        return *this;
    }

    return AddHeader(NNetwork::HEADER_CONTENT_TYPE, value);
}

bool TAppHostHttpProxyRequestBuilder::HasHeader(TStringBuf header) const {
    return AnyOf(Request_.GetHeaders(),
                 [header](const auto& h) { return h.GetName() == header; });
}

} // namespace NAlice::NAppHostRequest
