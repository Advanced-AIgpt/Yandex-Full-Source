#include "ping_handler.h"

#include "application.h"

namespace NPersonalCards {

bool TPingHandler::DoHttpReply(
    const TParsedHttpFull&,
    const TRequestReplier::TReplyParams& params
) {
    LOG(INFO) << "New ping request" << Endl;

    params.Output << THttpResponse(HTTP_OK) << "pong";
    params.Output.Flush();
    return true;
}

void TPingHandler::RegisterHttpHandlers(THttpHandlersMap* handlers) {
    auto factory = []() {
        static IHttpRequestHandler::TPtr handler = new TPingHandler;
        return handler;
    };
    handlers->insert(std::make_pair(TStringBuf("/ping"), factory));
}

} // namespace NPersonalCards
