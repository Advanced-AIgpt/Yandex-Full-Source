#include "ping_handler.h"

#include <library/cpp/svnversion/svnversion.h>

#include <util/string/builder.h>

constexpr TStringBuf TEXT_CONTENT_TYPE = "text/plain; charset=utf-8";

namespace NBASS {

bool TPingHandler::DoHttpReply(NBASS::TGlobalContextPtr, const TParsedHttpFull&,
                                  const TRequestReplier::TReplyParams& params)
{
    params.Output << THttpResponse(HTTP_OK).SetContentType(TEXT_CONTENT_TYPE).SetContent("pong");
    params.Output.Flush();
    return true;
}

void TPingHandler::RegisterHttpHandlers(THttpHandlersMap* handlers) {
    auto factory = []() {
        static IHttpRequestHandler::TPtr handler = MakeIntrusive<TPingHandler>();
        return handler;
    };
    handlers->Add(TStringBuf("/ping"), factory);
}

} // namespace NBASS
