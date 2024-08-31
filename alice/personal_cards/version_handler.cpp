#include "version_handler.h"

#include "application.h"

#include <library/cpp/json/json_writer.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/string/builder.h>

namespace NPersonalCards {

bool TVersionHandler::DoHttpReply(const TParsedHttpFull&,
                                   const TRequestReplier::TReplyParams& params) {
    NJson::TJsonMap ret;
    ret["svn_revision"] = TStringBuilder() << GetArcadiaSourceUrl() << "@" << GetArcadiaLastChange();
    params.Output << THttpResponse(HTTP_OK).SetContentType("application/json") << NJson::WriteJson(
        ret,
        false, // formatOutput
        false, // sortkeys
        false  // validateUtf8
    );
    params.Output.Flush();
    return true;
}

void TVersionHandler::RegisterHttpHandlers(THttpHandlersMap* handlers) {
    auto factory = []() {
        static IHttpRequestHandler::TPtr handler = new TVersionHandler;
        return handler;
    };
    handlers->insert(std::make_pair(TStringBuf("/version"), factory));

    // Balancer healthy check just need 200 so version page should work fine.
    handlers->insert(std::make_pair(TStringBuf("/slb_check"), factory));
}

} // namespace NPersonalCards
