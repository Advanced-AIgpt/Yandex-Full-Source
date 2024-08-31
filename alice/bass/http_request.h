#pragma once

#include <alice/bass/libs/globalctx/fwd.h>
#include <alice/bass/util/error.h>

#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/http/server/response.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <chrono>
#include <functional>

// Interface class for HTTP request handlers for specific path.
class IHttpRequestHandler : public TThrRefBase {
public:
    using TPtr = TIntrusivePtr<IHttpRequestHandler>;

public:
    virtual bool DoHttpReply(NBASS::TGlobalContextPtr globalCtx,
                             const TParsedHttpFull& httpRequest,
                             const TRequestReplier::TReplyParams& params) = 0;
};

class THttpHandlersMap {
public:
    using THandlerFactory = std::function<IHttpRequestHandler::TPtr()>;

public:
    THttpHandlersMap(NBASS::TGlobalContextPtr globalCtx);

    const THandlerFactory* ByPath(TStringBuf path) const;

    void Add(TStringBuf path, THandlerFactory factory);

private:
    THashMap<TString, THandlerFactory> Handlers;
};

class TLoggingHttpRequestHandler : public IHttpRequestHandler {
public:
    bool DoHttpReply(NBASS::TGlobalContextPtr globalCtx,
                     const TParsedHttpFull& httpRequest,
                     const TRequestReplier::TReplyParams& params) override;

protected:
    virtual THttpResponse DoTextReply(NBASS::TGlobalContextPtr globalCtx, const TString& requestText,
                                      const TParsedHttpFull& httpRequest, const THttpHeaders& httpHeaders) = 0;

    virtual const TString& GetReqIdClass() const = 0;
};

// Base class for JSON handlers that have no CGI parameters and both request and response are
// JSON objects.
class TJsonHttpRequestHandler : public IHttpRequestHandler {
public:
    bool DoHttpReply(NBASS::TGlobalContextPtr globalCtx,
                     const TParsedHttpFull& httpRequest,
                     const TRequestReplier::TReplyParams& params) override;

    virtual HttpCodes DoJsonReply(NBASS::TGlobalContextPtr globalCtx, const NSc::TValue& request,
                                  const TParsedHttpFull& httpRequest, const THttpHeaders& httpHeaders,
                                  NSc::TValue* response) = 0;

protected:
    virtual const TString& GetReqIdClass() const = 0;
};

class TMegamindProtocolHttpRequestHandler : public IHttpRequestHandler {
public:
    bool DoHttpReply(NBASS::TGlobalContextPtr globalCtx,
                     const TParsedHttpFull& httpRequest,
                     const TRequestReplier::TReplyParams& params) override;

    virtual HttpCodes DoJsonReply(NBASS::TGlobalContextPtr globalCtx, const TString& request,
                                  const TParsedHttpFull& httpRequest, const THttpHeaders& httpHeaders,
                                  TString& response) = 0;

protected:
    virtual const TString& GetReqIdClass() const = 0;
};

// HTTP requests handler, server creates new instance of this for each incoming
// request. The instances are created on the thread from threading pool.
class THttpReplier : public TRequestReplier {
public:
    THttpReplier(NBASS::TGlobalContextPtr globalCtx, const THttpHandlersMap& handlers);

    bool DoReply(const TReplyParams& params) override;

private:
     const THttpHandlersMap& Handlers;
     const std::chrono::time_point<std::chrono::steady_clock> ConstructedAt;
     NBASS::TGlobalContextPtr GlobalCtx;
};

bool CheckBalancerReqid(TStringBuf reqId);
