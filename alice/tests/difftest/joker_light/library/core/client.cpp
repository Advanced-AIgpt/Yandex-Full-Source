#include "client.h"
#include "context.h"
#include "session.h"
#include "stub_id.h"

#include <alice/joker/library/log/log.h>

#include <library/cpp/http/server/response.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/neh.h>

namespace NAlice::NJokerLight {

namespace {

using TReplyParams = TRequestReplier::TReplyParams;

constexpr TStringBuf CRLF = "\r\n";

bool ReplyError(const TError& error, THttpOutput& output, HttpCodes httpCode) {
    LOG(ERROR) << "DoReply: " << error << Endl;
    THttpResponse(httpCode).SetContentType("text/plain").SetContent(error.AsString()).OutTo(output);
    return true;
}

bool ReplyServerError(const TError& error, THttpOutput& output) {
    return ReplyError(error, output, HTTP_INTERNAL_SERVER_ERROR);
}

bool ReplyBadRequest(const TError& error, THttpOutput& output) {
    return ReplyError(error, output, HTTP_BAD_REQUEST);
}

bool ReplyStubProblem(const TError& error, THttpOutput& output) {
    return ReplyError(error, output, HTTP_I_AM_A_TEAPOT);
}

TString MakeProxyUri(const TString& proxyHostPort) {
    return TStringBuilder() << TStringBuf("full://") << proxyHostPort;
}

TErrorOr<NNeh::TResponseRef> ObtainViaRemoteRequest(const THttpContext& httpCtx, const TDuration timeout, TString& realAddr) {
    try {
        const TString& uri{httpCtx.ForwardUriAsString()};
        NNeh::NHttp::ERequestType reqType = NNeh::NHttp::ERequestType::Get;
        if (!TryFromString<NNeh::NHttp::ERequestType>(httpCtx.Http().Method, reqType)) {
            return TError{} << "Unsupported request type: " << httpCtx.Http().Method;
        }

        NNeh::TMessage msg = NNeh::TMessage::FromString(uri);
        TStringBuilder headersStr;
        TStringBuf contentType = "";
        for (const auto& header : httpCtx.Headers()) {
            if (!IsServiceHeader(header.Name())) {
                if (!headersStr.Empty()) {
                    headersStr << CRLF;
                }

                if (AsciiEqualsIgnoreCase(header.Name(), "Host")) {
                    headersStr << "Host: " << httpCtx.ForwardUriHostPort();
                } else {
                    headersStr << header.ToString();
                }

                if (AsciiEqualsIgnoreCase(header.Name(), "Content-Type")) {
                    contentType = header.Value();
                }
            }
        }

        NNeh::NHttp::ERequestFlags flags;
        TMaybe<TString> forwardProxy;
        const auto& proxyHeader = httpCtx.ProxyHeader();
        if (proxyHeader) {
            if (proxyHeader->find("joker") == TString::npos && proxyHeader->find("localhost") == TString::npos) {
                LOG(INFO) << "Remote request needs proxy: " << proxyHeader->Quote() << ", do redefine" << Endl;
                forwardProxy = MakeProxyUri(*proxyHeader);
                flags |= NNeh::NHttp::ERequestFlag::AbsoluteUri;
            } else {
                LOG(INFO) << "Remote request doesn't need proxy, don't redefine" << Endl;
            }
        }

        if (!NNeh::NHttp::MakeFullRequest(msg, headersStr, httpCtx.Body(), contentType, reqType, flags)) {
            return TError{} << "Unable to create original request: " << httpCtx.ForwardUri();
        }
        realAddr = msg.Addr;
        if (forwardProxy) {
            msg.Addr = std::move(*forwardProxy);
        }

        LOG(INFO) << "Remote request: " << uri << Endl;
        NNeh::TResponseRef r = NNeh::Request(msg)->Wait(timeout);
        if (r) {
            if (r->FirstLine.Contains("Bad request")) {
                LOG(INFO) << "Remote request is done: '" << uri << "': " << r->Duration << ", " << r->FirstLine << ", size: " << r->Data.size() << ", " << r->Data << Endl;
            } else {
                LOG(INFO) << "Remote request is done: '" << uri << "': " << r->Duration << ", " << r->FirstLine << ", size: " << r->Data.size() << Endl;
            }
            return r;
        }

        LOG(ERROR) << "Remote source timed out: " << uri << Endl;
        return TError{} << "Remote source timed out";
    } catch (yexception& e) {
        return TError{} << "Caught exception: " << e.what();
    }
}

bool AdminHandler(THttpServer& httpServer, const THttpContext& httpCtx) {
    const TStringBuf action = httpCtx.Cgi.Get("action");
    if (action == TStringBuf("shutdown")) {
        THttpResponse{HTTP_OK}.SetContentType("text/plain").SetContent("Shutdown initiated").OutTo(httpCtx.Output);
        httpServer.Shutdown();
        return true;
    } else if (action == TStringBuf("ping")) {
        THttpResponse{HTTP_OK}.SetContentType("text/plain").SetContent("pong").OutTo(httpCtx.Output);
        return true;
    }

    THttpResponse{HTTP_OK}
        .SetContentType("text/plain")
        .SetContent(TString::Join("Admin action ", TString{action}.Quote(), " has not implemented yet"))
        .OutTo(httpCtx.Output);
    return true;
}

bool SessionHandler(TContext& ctx, const THttpContext& httpCtx) {
    const auto& cgi = httpCtx.Cgi;

    const TString& id = cgi.Get("id");
    TSession session{ctx, id};

    TSession::TSettings settings;
    settings.FetchIfNotExists = cgi.Has("fetch_if_not_exists", "1");
    settings.ImitateDelay = cgi.Has("imitate_delay", "1");
    settings.DontSave = cgi.Has("dont_save", "1");

    session.Init(settings);

    THttpResponse{HTTP_OK}
        .SetContentType("text/plain")
        .SetContent(TString::Join("Session ", id.Quote(), " initialized"))
        .OutTo(httpCtx.Output);
    return true;
}

bool HttpHandler(TContext& ctx, const THttpContext& httpCtx) {
    LOG(DEBUG) << "Obtain stub for " << httpCtx.ForwardUriAsString().Quote() << Endl;

    const auto timeout = ctx.Config().SourceTimeout();
    TInstant startInstant = TInstant::Now();

    // Load session
    TCgiParameters cgi{httpCtx.JokerHeader().GetRef()};
    const TString& sessiodId = cgi.Get("sess");

    TSession session{ctx, sessiodId};
    if (const auto error = session.Load()) {
        return ReplyBadRequest(*error, httpCtx.Output);
    }
    const auto &settings = session.Settings();

    // Obtain stub
    auto& ydb = ctx.Ydb();
    const TString& reqId = cgi.Get("test");
    TString hash = httpCtx.CreateRequestHash();
    TMaybe<NYdb::TResultSetParser> stubParser = ydb.ObtainStub({sessiodId, reqId, hash});

    if (stubParser.Defined()) {
        LOG(DEBUG) << "Found stub for request" << Endl;

        // immediatelly return if have had errors previously
        TMaybe<TString> stubError = stubParser->ColumnParser("error").GetOptionalString();
        if (stubError.Defined()) {
            return ReplyStubProblem(TError{} << stubError, httpCtx.Output);
        }

        // imitate delay if needed
        if (settings.ImitateDelay) {
            TDuration stubTime = TDuration::MilliSeconds(stubParser->ColumnParser("duration_ms").GetOptionalUint64().GetRef());
            TDuration passedTime = TInstant::Now() - startInstant;

            if (stubTime > passedTime) {
                TDuration delay = stubTime - passedTime;
                LOG(DEBUG) << "Sleep " << delay << " before giving stub" << Endl;
                Sleep(delay);
            }
        }

        httpCtx.Output << stubParser->ColumnParser("data").GetOptionalString();
        httpCtx.Output.Flush();
        return true;
    } else {
        LOG(DEBUG) << "Didn't find stub for request" << Endl;

        // load new stub if we can
        if (!settings.FetchIfNotExists) {
            return ReplyStubProblem(TError{} << "Not allowed to fetch new stub for " << reqId.Quote(), httpCtx.Output);
        }

        TString realAddr;
        TErrorOr<NNeh::TResponseRef> errorOrResponse = ObtainViaRemoteRequest(httpCtx, timeout, realAddr);
        if (std::holds_alternative<TError>(errorOrResponse)) {
            // write error
            TError error = std::get<TError>(errorOrResponse);
            if (!settings.DontSave) {
                ydb.SaveStubError({sessiodId, reqId, hash}, error);
            }

            return ReplyStubProblem(error, httpCtx.Output);
        } else {
            // write stub
            NNeh::TResponseRef r = std::get<NNeh::TResponseRef>(errorOrResponse);

            TStringStream responseStream;
            responseStream << r->FirstLine << CRLF;
            r->Headers.OutTo(&responseStream);
            responseStream << CRLF << r->Data;

            const TString& data = responseStream.Str();
            if (!settings.DontSave) {
                ydb.SaveStub({sessiodId, reqId, hash}, r, data, realAddr);
            }

            httpCtx.Output << data;
            httpCtx.Output.Flush();
            return true;
        }
    }

    return true;
}

} // namespace

TClient::TClient(TContext& context)
    : Context_{context}
{
}

bool TClient::DoReply(const TReplyParams& params) {
    TLogging::InitTlsUniqId();

    class DoReplyGuard {
    public:
        DoReplyGuard() {
            Now_ = TInstant::Now();
            LOG(INFO) << "DoReply started " << Now_ << Endl;
        }

        ~DoReplyGuard() {
            TInstant currentNow = TInstant::Now();
            LOG(INFO) << "DoReply ended " << currentNow << ", time taken " << currentNow - Now_ << Endl;
        }

    private:
        TInstant Now_;
    } doReplyGuard;

    THttpContext httpCtx{params.Input, params.Output};

    try {
        LOG(DEBUG) << "Http request: " << httpCtx.FirstLine.Quote() << Endl;
        LOG(DEBUG) << httpCtx.CreateHeadersDescription() << Endl;

        const TStringBuf path{httpCtx.Uri().GetField(NUri::TField::EField::FieldPath)};

        if (path == TStringBuf("/admin")) {
            return AdminHandler(*HttpServ(), httpCtx);
        } else if (path == TStringBuf("/session")) {
            return SessionHandler(Context_, httpCtx);
        } else if (path == TStringBuf("/http") || path == TStringBuf("/")) {
            return HttpHandler(Context_, httpCtx);
        }
        return ReplyBadRequest(TError{} << "No handler found for: " << httpCtx.UriAsString().Quote(), httpCtx.Output);
    } catch (yexception& e) {
        return ReplyServerError(TError{} << "Caught exception: " << e.what(), httpCtx.Output);
    }

    return true;
}

} // namespace NAlice::NJokerLight
