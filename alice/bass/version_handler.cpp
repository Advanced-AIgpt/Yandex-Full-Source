#include "version_handler.h"

#include <alice/library/network/common.h>

#include <library/cpp/svnversion/svnversion.h>

#include <util/string/builder.h>

bool TVersionHandler::DoHttpReply(NBASS::TGlobalContextPtr /* globalCtx */, const TParsedHttpFull&,
                                  const TRequestReplier::TReplyParams& params)
{
    NSc::TValue ret;
    ret["svn_revision"].SetString(TStringBuilder() << GetArcadiaSourceUrl() << '@' << GetArcadiaLastChange());
    params.Output << THttpResponse(HTTP_OK).SetContentType(NAlice::NContentTypes::APPLICATION_JSON) << ret.ToJson();
    params.Output.Flush();
    return true;
}

void TVersionHandler::RegisterHttpHandlers(THttpHandlersMap* handlers) {
    auto factory = []() {
        static IHttpRequestHandler::TPtr handler = MakeIntrusive<TVersionHandler>();
        return handler;
    };
    handlers->Add(TStringBuf("/version"), factory);

    // Balancer healthy check just need 200 so version page should work fine.
    handlers->Add(TStringBuf("/slb_check"), factory);
}

bool TVersionJsonHandler::DoHttpReply(NBASS::TGlobalContextPtr /* globalCtx */, const TParsedHttpFull&,
                                  const TRequestReplier::TReplyParams& params)
{
    NSc::TValue result;
    result["branch"].SetString(GetBranch());
    result["tag"].SetString(GetTag());
    params.Output << THttpResponse(HTTP_OK).SetContentType(NAlice::NContentTypes::APPLICATION_JSON) << result.ToJson();
    params.Output.Flush();
    return true;
}

void TVersionJsonHandler::RegisterHttpHandlers(THttpHandlersMap* handlers) {
    auto factory = []() {
        static IHttpRequestHandler::TPtr handler = MakeIntrusive<TVersionJsonHandler>();
        return handler;
    };
    handlers->Add(TStringBuf("/version_json"), factory);
}
