#pragma once

#include <library/cpp/http/io/headers.h>
#include <library/cpp/http/misc/httpcodes.h>

namespace NAlice {

class IHttpResponse {
public:
    virtual ~IHttpResponse() = default;

    virtual IHttpResponse& AddHeader(const THttpInputHeader& header) = 0;
    virtual IHttpResponse& SetContent(const TString& content) = 0;
    virtual IHttpResponse& SetContentType(TStringBuf contentType) = 0;
    virtual IHttpResponse& SetHttpCode(HttpCodes httpCode) = 0;
    virtual TString Content() const = 0;
    virtual HttpCodes HttpCode() const = 0;
    virtual const THttpHeaders& Headers() const = 0;

    IHttpResponse& AddHeader(const TString& name, const TString& value) {
        return AddHeader(THttpInputHeader{name, value});
    }

    void Flush();

protected:
    virtual void DoOut() const = 0;

private:
    bool IsFlushed_ = false;
};

} // namespace NAlice
