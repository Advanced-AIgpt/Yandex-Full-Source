#include "reopenlogs_handler.h"
#include "application.h"

#include <library/cpp/svnversion/svnversion.h>

#include <util/string/builder.h>

constexpr TStringBuf TEXT_CONTENT_TYPE = "text/plain; charset=utf-8";

namespace NBASS {

bool TReopenLogsHandler::DoHttpReply(NBASS::TGlobalContextPtr, const TParsedHttpFull&,
                                  const TRequestReplier::TReplyParams& params)
{
    TApplication::GetInstance()->ReopenLogs();

    params.Output << THttpResponse(HTTP_OK).SetContentType(TEXT_CONTENT_TYPE).SetContent("OK");
    params.Output.Flush();
    return true;
}

void TReopenLogsHandler::RegisterHttpHandlers(THttpHandlersMap* handlers) {
    auto factory = []() {
        static IHttpRequestHandler::TPtr handler = MakeIntrusive<TReopenLogsHandler>();
        return handler;
    };
    handlers->Add(TStringBuf("/reopenlogs"), factory);
}

} // namespace NBASS
