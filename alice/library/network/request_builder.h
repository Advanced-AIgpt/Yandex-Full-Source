#pragma once

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice::NNetwork {

class IRequestBuilder {
public:
    virtual ~IRequestBuilder() = default;

    virtual IRequestBuilder& AddCgiParams(const TCgiParameters& cgi) = 0;
    virtual IRequestBuilder& AddCgiParam(TStringBuf key, TStringBuf value) = 0;
    virtual IRequestBuilder& ReplaceCgiParams(const TCgiParameters& cgi) = 0;
    virtual IRequestBuilder& ReplaceCgiParam(TStringBuf key, TStringBuf value) = 0;
    virtual IRequestBuilder& SetProxy(const TString& proxy) = 0;
    virtual IRequestBuilder& AddHeader(TStringBuf key, TStringBuf value) = 0;
    virtual IRequestBuilder& SetMethod(TStringBuf method) = 0;
    virtual IRequestBuilder& SetBody(TStringBuf body, TStringBuf method) = 0;
    virtual IRequestBuilder& SetContentType(TStringBuf value) = 0;
    virtual IRequestBuilder& SetRequestLabel(const TString& label) = 0;

    virtual bool HasHeader(TStringBuf header) const = 0;
};

} // namespace NAlice::NNetwork
