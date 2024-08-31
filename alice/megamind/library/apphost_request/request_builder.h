#pragma once

#include "node.h"

#include <alice/library/apphost_request/request_builder.h>

namespace NAlice::NMegamind {

class TAppHostHttpProxyMegamindRequestBuilder : public NAppHostRequest::TAppHostHttpProxyRequestBuilder {
public:
    TAppHostHttpProxyMegamindRequestBuilder() = default;

    NAppHostHttp::THttpRequest& CreateAndPushRequest(IAppHostCtx& ahCtx, TStringBuf itemName, TStringBuf tagName={});
};

} // namespace NAlice::NMegamind
