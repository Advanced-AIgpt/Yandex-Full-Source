#pragma once

#include "http_request.h"

#include <library/cpp/tvmauth/client/facade.h>

namespace NBASS {

// HTTP handler for push notification (AssitantNotificationSystem) request.
class TPushRequestHandler : public IHttpRequestHandler {
public:
    TPushRequestHandler(NTvmAuth::TTvmClient& tvm)
        : Tvm_{tvm}
    {
    }

    bool DoHttpReply(TGlobalContextPtr globalCtx, const TParsedHttpFull& http,
                     const TRequestReplier::TReplyParams& params) override;

    static void RegisterHttpHandlers(THttpHandlersMap* handlers);

    NTvmAuth::TTvmClient& Tvm_;
};

} // namespace NBASS
