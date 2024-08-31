#pragma once

#include "http_request.h"

namespace NPersonalCards {

// HTTP handler for rotation logs request.
class TRotateLogsHandler : public IHttpRequestHandler {
public:
    bool DoHttpReply(const TParsedHttpFull&,
                      const TRequestReplier::TReplyParams& params) override;

    static void RegisterHttpHandlers(THttpHandlersMap* handlers);
};

} // namespace NPersonalCards
