#include "client.h"
#include "config.h"
#include "globalctx.h"
#include "http_session.h"
#include "joker.h"
#include "request.h"

#include <alice/joker/library/core/ctrl_session.h>
#include <alice/joker/library/log/log.h>
#include <alice/joker/library/status/status.h>
#include <alice/joker/library/stub/stub_id.h>

#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/http/server/response.h>
#include <library/cpp/uri/uri.h>

#include <util/generic/ptr.h>
#include <util/generic/serialized_enum.h>
#include <util/string/builder.h>
#include <util/string/join.h>

namespace NAlice::NJoker {
namespace {

class IAction {
public:
    virtual ~IAction() = default;

    virtual TStatus AppendConstructData(TStringBuf chunk) = 0;
    virtual TStatus Do(THttpProxySession& session) = 0;
};

class TRegisterAction : public IAction {
public:
    TStatus AppendConstructData(TStringBuf chunk) override {
        if (StubId_.Defined()) {
            return TError{TError::EType::Logic} << "Invalid format for command sync";
        }

        TErrorOr<TStubId> stubRequest = TStubId::Create(chunk, &Version_);
        if (TError* error = std::get_if<TError>(&stubRequest)) {
            return std::move(*error);
        }

        StubId_.ConstructInPlace(std::move(*std::get_if<TStubId>(&stubRequest)));

        return Success();
    }

    TStatus Do(THttpProxySession& session) override {
        if (!StubId_) {
            return TError{TError::EType::Logic} << "Invalid format of sync command (empty)";
        }

        if (Version_.Empty()) {
            return TError{TError::EType::Logic} << "Invalid format of sync command (no version found)";
        }

        return session.SyncVersionedStub(*StubId_, Version_);
    }

private:
    TMaybe<TStubId> StubId_;
    TString Version_;
};

TStatus StubAction(TStringBuf requestLine, THttpProxySession& session) {
    TStatus status;
    THolder<IAction> action;
    auto consumer = [&status, &action](TStringBuf chunk) -> bool {
        if (!action) {
            if (chunk == TStringBuf("sync")) {
                action = MakeHolder<TRegisterAction>();
            } else if (chunk == TStringBuf("touch")) {
                action = MakeHolder<TRegisterAction>();
            } else if (chunk == TStringBuf("force_update")) {
                return false; // FIXME
            } else {
                status = TError{TError::EType::Logic} << "Unknown or unsupported action: " << chunk;
                return false;
            }
            return true;
        }

        status = action->AppendConstructData(chunk);
        return status.Defined() ? false : true;
    };

    StringSplitter(requestLine).Split('\t').Consume(consumer);

    if (!status.Defined()/* no error */) {
        if (action) {
            status = action->Do(session);
        } else {
            status.ConstructInPlace(TError{HTTP_BAD_REQUEST} << "Empty request line");
        }
    }

    return status;
}

TErrorOr<TStubId> StubRequestViaProxy(TGlobalContext& globalCtx, THttpContext& httpCtx, const TString& header, THttpProxySession*& session) {
    TCgiParameters cgi{header};

    auto groupId = cgi.Find("group_id");
    if (groupId != cgi.end()) {
        globalCtx.RequestsHistory().Add(groupId->second, httpCtx);
    }

    if (session = THttpProxySession::GetSession(cgi.Get("sess")); !session) {
        return TError{HTTP_I_AM_A_TEAPOT} << "session " << cgi.Get("sess").Quote() << " doesn't exists";
    }
    return TStubId::Load(cgi, httpCtx.CreateRequestHash(globalCtx.Config()));
}

bool AdminHandler(THttpServer& httpServer, THttpContext& httpContext) {
    const TStringBuf action = httpContext.Cgi.Get("action");
    if (action == TStringBuf("shutdown")) {
        THttpResponse(HTTP_OK).SetContentType("text/plain").SetContent("Shutdown initiated").OutTo(httpContext.Output);
        httpServer.Shutdown(); // graceful shutdown
        return true;
    } else if (action == TStringBuf("ping")) {
        THttpResponse(HTTP_OK).SetContentType("text/plain").SetContent("pong").OutTo(httpContext.Output);
        return true;
    }

    THttpResponse(HTTP_OK).SetContentType("text/plain").SetContent(TStringBuilder() << "Admin action " << TString{action}.Quote() << " has not implemented yet").OutTo(httpContext.Output);
    return true;
}

// FIXME add error handlers for each action
// FIXME data should be stream (read http input)
NSc::TValue SyncVersionedStubs(TStringBuf data, THttpProxySession& session) {
    NSc::TValue actions;
    auto onEachRequest = [&actions, &session](TStringBuf line) {
        NSc::TValue& item = actions.Push();
        item["data"].SetString(line);

        if (const TStatus error = StubAction(line, session)) {
            item["error"].SetString(error->AsString());
        } else {
            item["success"].SetBool(true);
        }
    };

    if (!data.Empty()) {
        StringSplitter(data).Split('\n').Consume(onEachRequest);
    }

    return actions;
}

bool SyncVersionedHandler(THttpContext& httpContext) {
    const TString& sessionId{httpContext.Cgi.Get("session_id")};
    THttpProxySession* session = THttpProxySession::GetSession(TSessionId{sessionId});
    if (!session) {
        THttpContext::ErrorResponse(httpContext.Output, TError{HTTP_BAD_REQUEST} << "No registered session " << sessionId.Quote() << " found");
        return true;
    }
    NSc::TValue response;
    NSc::TValue actions = SyncVersionedStubs(httpContext.Body(), *session);
    // FIXME make separate function, add error checking
    if (!actions.IsNull()) {
        response["actions"] = std::move(actions);
    }
    THttpResponse(HTTP_OK).SetContentType("application/json").SetContent(response.ToJson()).OutTo(httpContext.Output);
    return true;
}

bool SessionHandler(TGlobalContext& globalCtx, THttpContext& httpContext) {
    THttpProxySession::TFlags flags;
    for (const auto& kv : GetEnumNames<THttpProxySession::EFlag>()) {
        const TString& cgiValue = httpContext.Cgi.Get(kv.second);
        if (cgiValue == TStringBuf("1")) {
            flags |= kv.first;
        }
    }

    THttpProxySession* session = THttpProxySession::NewSession(TSessionId{httpContext.Cgi.Get("id")}, globalCtx, flags);
    if (!session) {
        THttpContext::ErrorResponse(httpContext.Output, TError{HTTP_BAD_REQUEST} << "Session with " << httpContext.Cgi.Get("id").Quote() << " has already registered");
        return true;
    }

    NSc::TValue response;
    response["session_id"].SetString(session->Id());
    NSc::TValue actions = SyncVersionedStubs(httpContext.Body(), *session);
    if (!actions.ArrayEmpty()) {
        response["actions"] = std::move(actions);
    }
    THttpResponse(HTTP_OK).SetContentType("application/json").SetContent(response.ToJson()).OutTo(httpContext.Output);

    return true;
}

bool PushHandler(TGlobalContext& globalCtx, THttpContext& httpContext) {
    const TString& sessionId{httpContext.Cgi.Get("session_id")};

    THolder<TSessionControl> session;
    if (const TStatus error = TSessionControl::Load(TSessionId{sessionId}, globalCtx.Config(), session)) {
        THttpContext::ErrorResponse(httpContext.Output, TError{HTTP_BAD_REQUEST} << "No registered session " << sessionId.Quote() << " found");
        return true;
    }

    TStringStream out;
    if (session->Push(globalCtx, out)) {
        THttpContext::ErrorResponse(httpContext.Output, TError{HTTP_INTERNAL_SERVER_ERROR} << "Can't push session " << sessionId.Quote() << " stubs");
        return true;
    }

    THttpResponse(HTTP_OK).SetContentType("text/plain").SetContent(out.Str()).OutTo(httpContext.Output);
    return true;
}

bool HistoryHandler(TGlobalContext& globalCtx, THttpContext& httpContext) {
    const TString& groupId{httpContext.Cgi.Get("group_id")};
    TRequestsHistory& requestsHistory = globalCtx.RequestsHistory();

    TMaybe<TRequestsHistory::TRequestEntries> history = requestsHistory.Get(groupId);
    if (history.Empty()) {
        THttpContext::ErrorResponse(httpContext.Output, TError{HTTP_BAD_REQUEST} << "No history for group id " << groupId.Quote() << " found");
        return true;
    }

    TString historyJson = TRequestsHistory::ToJson(history.GetRef());
    THttpResponse(HTTP_OK).SetContentType("application/json").SetContent(historyJson).OutTo(httpContext.Output);
    return true;
}

} // namespace

TClient::TClient(TGlobalContext& globalCtx)
    : GlobalCtx_{globalCtx}
{
}

namespace {

using TReplyParams = TRequestReplier::TReplyParams;

bool ReplyError(const TError& error, HttpCodes httpCode, const TReplyParams& params) {
    LOG(ERROR) << "DoReply: " << error << Endl;
    THttpResponse(httpCode).SetContentType("text/plain").SetContent(ToString(error)).OutTo(params.Output);
    return true;
}

bool ReplyBadRequest(const TError& error, const TReplyParams& params) {
    return ReplyError(error, HTTP_BAD_REQUEST, params);
}

bool ReplyServerError(const TError& error, const TReplyParams& params) {
    return ReplyError(error, HTTP_INTERNAL_SERVER_ERROR, params);
}

bool ReplyStubProblem(const TError& error, const TReplyParams& params) {
    return ReplyError(error, HTTP_I_AM_A_TEAPOT, params);
}

} // namespace

bool TClient::DoReply(const TReplyParams& params) {
    TLogging::InitTlsUniqId();

    THttpContext httpCtx(params.Input, params.Output);

    try {
        LOG(DEBUG) << "Http request: " << httpCtx.FirstLine << Endl;

        const TMaybe<TString>& jh = httpCtx.JokerHeader();
        if (!jh.Defined()) {
            const TStringBuf path{httpCtx.Uri().GetField(NUri::TField::EField::FieldPath)};

            if (path == TStringBuf("/admin")) {
                return AdminHandler(*HttpServ(), httpCtx);
            } else if (path == TStringBuf("/session")) {
                return SessionHandler(GlobalCtx_, httpCtx);
            } else if (path == TStringBuf("/sync_versioned")) {
                return SyncVersionedHandler(httpCtx);
            } else if (path == TStringBuf("/push")) {
                return PushHandler(GlobalCtx_, httpCtx);
            } else if (path == TStringBuf("/history")) {
                return HistoryHandler(GlobalCtx_, httpCtx);
            }
            return ReplyBadRequest(TError{HTTP_NOT_FOUND} << "No handler found for: " << httpCtx.UriAsString(), params);
        }

        THttpProxySession* session = nullptr;
        TErrorOr<TStubId> stubId = StubRequestViaProxy(GlobalCtx_, httpCtx, *jh, session);
        if (const TError* error = std::get_if<TError>(&stubId)) {
            return ReplyStubProblem(*error, params);
        }

        Y_ASSERT(session);

        /* FIXME (petrk) Implement it.
        if (httpCtx.ViaProxy()) {
            stubRequest->SetProxy(*httpCtx.ViaProxy());
        }
        */

        if (const auto error = session->ReplyTo(httpCtx, std::get<TStubId>(stubId))) {
            return ReplyServerError(*error, params);
        }
    } catch (yexception& e) {
        return ReplyServerError(TError{} << "Caught exception: " << e.what(), params);
    }

    return true;
}

} // namespace NAlice::NJoker
