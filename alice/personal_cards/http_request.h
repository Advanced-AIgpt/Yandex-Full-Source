#pragma once

#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/http/server/response.h>
#include <library/cpp/json/json_reader.h>

namespace NPersonalCards {

// Interface class for HTTP request handlers for specific path.
class IHttpRequestHandler : public TThrRefBase {
public:
    using TPtr = TIntrusivePtr<IHttpRequestHandler>;

    virtual bool DoHttpReply(const TParsedHttpFull& httpRequest,
                              const TRequestReplier::TReplyParams& params) = 0;
};

// Base class for JSON handlers that have no CGI parameters and both request and response are
// JSON objects.
class TJsonHttpRequestHandler : public IHttpRequestHandler {
public:
    bool DoHttpReply(const TParsedHttpFull& httpRequest,
                      const TRequestReplier::TReplyParams& params) override;

    virtual HttpCodes DoJsonReply(NJson::TJsonMap&& request, NJson::TJsonMap* response,
                                   const TParsedHttpFull& httpRequest, const THttpHeaders& httpHeaders) = 0;

protected:
    virtual const TString& GetReqIdClass() const = 0;
};

using THttpRequestHandlerFactory = std::function<IHttpRequestHandler::TPtr()>;
using THttpHandlersMap = THashMap<TStringBuf, THttpRequestHandlerFactory>;

// HTTP requests handler, server creates new instance of this for each incoming
// request. The instances are created on the thread from threading pool.
class THttpReplier : public TRequestReplier {
public:
    bool DoReply(const TReplyParams& params) override;
};

} // namespace NPersonalCards
