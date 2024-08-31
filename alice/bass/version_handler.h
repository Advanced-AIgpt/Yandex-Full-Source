#pragma once

#include "http_request.h"

// HTTP handler for service version request.
class TVersionHandler : public IHttpRequestHandler {
public:
    bool DoHttpReply(NBASS::TGlobalContextPtr globalCtx, const TParsedHttpFull&,
                     const TRequestReplier::TReplyParams& params) override;

    static void RegisterHttpHandlers(THttpHandlersMap* handlers);
};

// HTTP version_json handler for Alice integration tests.
class TVersionJsonHandler : public IHttpRequestHandler {
public:
    bool DoHttpReply(NBASS::TGlobalContextPtr globalCtx, const TParsedHttpFull&,
                     const TRequestReplier::TReplyParams& params) override;

    static void RegisterHttpHandlers(THttpHandlersMap* handlers);
};
