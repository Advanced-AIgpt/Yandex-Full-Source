#include "rotate_logs_handler.h"

#include <alice/bass/libs/logging/logger.h>

namespace NPersonalCards {

bool TRotateLogsHandler::DoHttpReply(const TParsedHttpFull&, const TRequestReplier::TReplyParams& params) {
    LOG(INFO) << "Rotate logs requested" << Endl;
    TLogging::Rotate();
    LOG(INFO) << "Logs rotated" << Endl;
    params.Output << THttpResponse(HTTP_OK).SetContentType("text/plain") << "Sucess";
    return true;
}

void TRotateLogsHandler::RegisterHttpHandlers(THttpHandlersMap* handlers) {
    auto factory = []() {
        static IHttpRequestHandler::TPtr handler = new TRotateLogsHandler;
        return handler;
    };
    handlers->insert(std::make_pair(TStringBuf("/rotate_logs"), factory));
}

} // namespace NPersonalCards
