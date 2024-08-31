#pragma once

#include "item_adapter.h"

#include <alice/megamind/library/util/http_response.h>
#include <alice/megamind/library/util/status.h>

#include <library/cpp/http/io/headers.h>

#include <apphost/api/service/cpp/service_context.h>

namespace NAlice::NMegamind {

class TAppHostHttpResponse : public IHttpResponse {
public:
    explicit TAppHostHttpResponse(TItemProxyAdapter& itemProxyAdapter)
        : ItemProxyAdapter_{itemProxyAdapter}
    {
    }

    // IHttpResponse overrides:
    IHttpResponse& AddHeader(const THttpInputHeader& header) override;
    IHttpResponse& SetContent(const TString& content) override;
    IHttpResponse& SetContentType(TStringBuf contentType) override;
    IHttpResponse& SetHttpCode(HttpCodes httpCode) override;
    TString Content() const override;
    HttpCodes HttpCode() const override;
    const THttpHeaders& Headers() const override;

protected:
    void DoOut() const final override;

private:
    HttpCodes HttpCode_ = HTTP_OK;
    TString ContentType_;
    TString Content_;
    THttpHeaders Headers_;
    TItemProxyAdapter& ItemProxyAdapter_;
};

} // namespace NAlice::NMegamind
