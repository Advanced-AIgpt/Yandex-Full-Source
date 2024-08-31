#pragma once

#include "http_request.h"

namespace NPersonalCards {

// HTTP handler for service ping request.
class TPingHandler : public IHttpRequestHandler {
public:
    bool DoHttpReply(
        const TParsedHttpFull&,
        const TRequestReplier::TReplyParams& params
    ) override;

    static void RegisterHttpHandlers(THttpHandlersMap* handlers);
};

} // namespace NPersonalCards
