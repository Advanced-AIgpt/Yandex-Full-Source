#pragma once

#include "http_request.h"

namespace NBASS {

class TPingHandler : public IHttpRequestHandler {
public:
    bool DoHttpReply(NBASS::TGlobalContextPtr globalCtx, const TParsedHttpFull& httpFull,
                     const TRequestReplier::TReplyParams& params) override;

    static void RegisterHttpHandlers(THttpHandlersMap* handlers);
};

} // namespace NBASS
