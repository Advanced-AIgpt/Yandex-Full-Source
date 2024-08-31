#pragma once

#include "http_request.h"

namespace NPersonalCards {

// HTTP handler for service version request.
class TVersionHandler : public IHttpRequestHandler {
public:
    bool DoHttpReply(const TParsedHttpFull&,
                      const TRequestReplier::TReplyParams& params) override;

    static void RegisterHttpHandlers(THttpHandlersMap* handlers);
};

} // namespace NPersonalCards
