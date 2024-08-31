#include "http_session.h"

#include "globalctx.h"
#include "request.h"

#include <alice/joker/library/log/log.h>
#include <alice/joker/library/stub/stub.h>

#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/neh.h>

#include <util/generic/scope.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/system/file.h>

namespace NAlice::NJoker {
namespace {

constexpr TStringBuf CRLF = "\r\n";

class TReplyToVisitor {
public:
    TReplyToVisitor(THttpContext& httpCtx)
        : HttpCtx_{httpCtx}
    {
    }

    TStatus operator()(const TError& error) {
        return error;
    }

    TStatus operator()(TStubItemPtr stubItem) {
        if (Y_UNLIKELY(!stubItem)) {
            return TError{TError::EType::Logic} << "Stub not found but must be there";
        }

        HttpCtx_.Output << stubItem->Get().GetResponse().GetData();
        HttpCtx_.Output.Flush();
        return Success();
    }

private:
    THttpContext& HttpCtx_;
};

void ImitateDelayIfNeeded(const THttpProxySession::TStubMeta& meta, TStubItemPtr result) {
    if (meta.ImitateDelay) {
        const auto& proto = result->Get();
        if (proto.HasDurationMs()) {
            const auto delay = proto.GetDurationMs();
            LOG(DEBUG) << "Imitate delay for " << delay << " milliseconds" << Endl;
            Sleep(TDuration::MilliSeconds(delay));
        }
    }
}

} // namespace

// THttpProxySession::TStubStats ----------------------------------------------
THttpProxySession::TStubWrapper::TStubWrapper(const TStubId& stubId, TStubMeta& stubMeta)
    : StubId_{stubId}
    , StubMeta_{stubMeta}
{
}

// THttpProxySession ----------------------------------------------------------
THashMap<TString, THttpProxySession> THttpProxySession::Sessions_;
TRWMutex THttpProxySession::SessionsLock_;

// static
THttpProxySession* THttpProxySession::NewSession(const TSessionId& id, TGlobalContext& globalCtx, TFlags flags) {
    THttpProxySession* session = nullptr;
    {
        TWriteGuard guard(SessionsLock_);

        if (Sessions_.contains(id)) {
            return nullptr;
        }

        session = &Sessions_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(static_cast<const TString&>(id)),
            std::forward_as_tuple(id, globalCtx, flags)
        ).first->second;
    }

    if (TStatus error = session->WriteInfoFile()) {
        LOG(ERROR) << *error << Endl;
        return nullptr;
    }
    return session;
}

// static
THttpProxySession* THttpProxySession::GetSession(const TSessionId& id) {
    TReadGuard guard(SessionsLock_);
    return Sessions_.FindPtr(id);
}

THttpProxySession::THttpProxySession(TSessionId sessionId, TGlobalContext& globalCtx, TFlags flags)
    : TSession{std::move(sessionId), globalCtx.Config()}
    , GlobalCtx_{globalCtx}
    , Flags_{flags}
    , RunLogWriter_{CurrentRunLogDirectory()}
{
}

TStatus THttpProxySession::SyncVersionedStub(const TStubId& stubId, const TString& version) {
    const TFsPath fileName{StubFileName(stubId, &version)};
    TFileStat fs;
    if (fileName.Exists() || !fileName.Stat(fs) || fs.Size == 0) {
        TStubItemPtr stubItem;

        if (auto error = Backend().ObtainStub(stubId, version, stubItem)) {
            return error;
        }

        if (auto error = SaveStub(stubId, &version, stubItem)) {
            return error;
        }
    } else {
        LOG(INFO) << "Stub '" << stubId.MakeKey(&version) << "' found in local cache" << Endl;
    }

    TStubMetaPtr stubMeta = MakeIntrusive<TStubMeta>(*this, stubId, version);
    const TString key{stubId.MakeKey(nullptr)};

    bool isInserted = false;
    with_lock (StubsLock_) {
        isInserted = Stubs_.emplace(key, stubMeta).second;
    }

    if (!isInserted) {
        return TError{TError::EType::Logic}.SetHttpCode(HTTP_BAD_REQUEST) << "Stub has already existed: " << key;
    }

    RunLogWriter_.Emplace<TVersionLogEntry>(stubId, version);

    return Success();
}

TStatus THttpProxySession::ObtainViaRemoteRequest(const TStubId& stubId, TDuration timeout, THttpContext& httpCtx, TStubItemPtr& stub) const {
    const TString& uri{httpCtx.UriAsString()};
    NNeh::NHttp::ERequestType reqType = NNeh::NHttp::ERequestType::Get;
    if (!TryFromString<NNeh::NHttp::ERequestType>(httpCtx.Http().Method, reqType)) {
        return TError{TError::EType::Logic} << "Unsupported request type: " << httpCtx.Http().Method;
    }

    NNeh::TMessage msg = NNeh::TMessage::FromString(uri);
    TStringBuilder headersStr;
    for (const auto& header : httpCtx.Headers()) {
        if (!headersStr.Empty()) {
            headersStr << CRLF;
        }
        headersStr << header.ToString();
    }
    if (!NNeh::NHttp::MakeFullRequest(msg, headersStr, httpCtx.Body(), TStringBuf(""), reqType)) {
        return TError{TError::EType::Logic} << "Unable to create original request: " << httpCtx.Uri();
    }

    LOG(INFO) << "Remote request: " << uri << Endl;
    NNeh::TResponseRef r = NNeh::Request(msg)->Wait(timeout);
    if (r) {
        LOG(INFO) << "Remote request is done: '" << uri << "': " << r->Duration << ", " << r->FirstLine << ", size: " << r->Data.size() << Endl;

        TStringStream responseStream;
        responseStream << r->FirstLine << CRLF;
        r->Headers.OutTo(&responseStream);
        responseStream << CRLF << r->Data;

        TStubItem::TProto response;
        response.SetVersion("1"); // TODO use constant
        response.MutableRequest()->SetUrl(r->Request.Addr);
        response.MutableRequest()->SetData(r->Request.Data);
        response.SetCreatedAtMs(TInstant::Now().MilliSeconds());
        response.SetDurationMs(r->Duration.MilliSeconds());
        response.MutableResponse()->SetFirstLine(r->FirstLine);
        response.MutableResponse()->SetData(responseStream.Str());
        stub = TStubItem::MakePtr(stubId, std::move(response));
        return Success();
    }

    LOG(ERROR) << "Remote request failed: " << uri << Endl;
    return TError{TError::EType::Logic} << "no response from remote backend";
}

TStatus THttpProxySession::SaveStub(const TStubId& stubId, const TString* version, TStubItemPtr item) const {
    const TFsPath fn = StubFileName(stubId, version);
    try {
        fn.Parent().MkDirs();
        TFileOutput out{fn};
        return item->Serialize(out);
    } catch (...) {
        return TError{TError::EType::Logic} << "Error during serialize to file '" << fn << "': " << CurrentExceptionMessage();
    }
}

THttpProxySession::TStubMetaPtr THttpProxySession::LoadOrCreateStubMeta(const TStubId& stubId) {
    TStubMetaPtr stubMeta;

    const TString uniqKey{stubId.MakeKey(nullptr)};
    with_lock (StubsLock_) {
        TStubMetaPtr* sm = Stubs_.FindPtr(uniqKey);
        if (sm) {
            stubMeta = *sm;
        } else {
            stubMeta = Stubs_.emplace(uniqKey, MakeIntrusive<TStubMeta>(*this, stubId)).first->second;
        }
    }

    return stubMeta;
}

TStatus THttpProxySession::ReplyTo(THttpContext& httpCtx, const TStubId& stubId) {
    TStubMetaPtr stubMeta = LoadOrCreateStubMeta(stubId);
    TStubWrapper stub{stubId, *stubMeta};

    return std::visit(TReplyToVisitor(httpCtx), ProcessRequest(httpCtx, stub));
}

TErrorOr<TStubItemPtr> THttpProxySession::ProcessRequest(THttpContext& httpCtx, TStubWrapper& stub) {
    TStubItemPtr result;
    const TString stubKey = stub.StubId().MakeKey(nullptr);

    bool hasRemoteRequest = false;
    const auto timeout = GlobalCtx_.Config().Server().SourceTimeout();

    auto onRemoteRequest = [this, &httpCtx, &result, &stub, &hasRemoteRequest, timeout]() -> TStatus {
        TStubItemPtr stubItem;
        if (auto rrError = ObtainViaRemoteRequest(stub.StubId(), timeout, httpCtx, stubItem)) {
            return rrError;
        }

        // Update stub in local fs cache as new.
        if (auto error = SaveStub(stub.StubId(), NewVersion, stubItem)) {
            return error;
        }

        result = std::move(stubItem);
        hasRemoteRequest = true;

        return Success();
    };

    if (const TStatus& error = stub.Meta().RemoteRequestIfNeeded(onRemoteRequest).GetValue(timeout)) {
        // A remote request completed but failed!
        RunLogWriter_.Emplace<TFailedLogEntry>(stub.StubId(), error->AsString(), httpCtx.FirstLine);
        return *error;
    }

    if (result) {
        // A remote request is successfully done and result has already have a valid stubItem.
        RunLogWriter_.Emplace<TCreatedLogEntry>(
            stub.StubId(),
            httpCtx.FirstLine,
            stub.Meta().Version,
            hasRemoteRequest
        );
        return result;
    }

    // Try to load a NEW stub for request which could be fetched in previous time.
    if (!LoadStub(stub.StubId(), NewVersion, result).Defined()) {
        // Using a previously fetched new version of stub.
        Y_ASSERT(result);
        RunLogWriter_.Emplace<TCreatedLogEntry>(
            stub.StubId(),
            httpCtx.FirstLine,
            stub.Meta().Version,
            false
        );
        LOG(DEBUG) << "Got stub from a previously fetched new version: " << stubKey << Endl;

        ImitateDelayIfNeeded(stub.Meta(), result);

        return result;
    }

    // Try to load from VERSIONED file.
    if (!stub.Meta().Version.Defined()) {
        // Neither NEW nor VERSION results are existed (no force_update and fetch_if_not_existed requested).
        RunLogWriter_.Emplace<TFailedLogEntry>(stub.StubId(), "No stub found", httpCtx.FirstLine);
        return TError{TError::EType::Logic} << "No stub found";
    }

    if (auto error = LoadStub(stub.StubId(), stub.Meta().Version.Get(), result)) {
        RunLogWriter_.Emplace<TFailedLogEntry>(stub.StubId(), error->AsString(), httpCtx.FirstLine);
        return *error;
    }

    Y_ASSERT(result);

    // VERSIONED file found.
    RunLogWriter_.Emplace<TVersionLogEntry>(stub.StubId(), *stub.Meta().Version, httpCtx.FirstLine);
    LOG(INFO) << "Got stub from versioned file: " << stubKey << Endl;

    // Imitate delay if needed, for stubs from local
    ImitateDelayIfNeeded(stub.Meta(), result);

    return result;
}

// THttpProxySession::TSubMeta ------------------------------------------------
THttpProxySession::TStubMeta::TStubMeta(const THttpProxySession& session, const TStubId& stubId, TMaybe<TString> version)
    : Version{std::move(version)}
    , NewVersionFileExists{session.StubFileName(stubId, NewVersion).Exists()}
    , WantNewResult{session.Flags_.HasFlags(EFlag::ForceUpdate)}
    , FetchIfNotExists{session.Flags_.HasFlags(EFlag::FetchIfNotExists)}
    , ImitateDelay{session.Flags_.HasFlags(EFlag::ImitateDelay)}
{
    LOG(DEBUG) << "MetaStubInfo Version("
               << Version << "), WantNewResult("
               << WantNewResult << "), FetchIfNotExists("
               << FetchIfNotExists << "), NewVersionFileExists("
               << NewVersionFileExists << "), ImitateDelay ("
               << ImitateDelay << ')'
               << Endl;
}

NThreading::TFuture<TStatus> THttpProxySession::TStubMeta::RemoteRequestIfNeeded(std::function<TStatus()> onRequest) {
    TNewResultPromise* forceNewPromise = nullptr;
    TNewResultPromise* statusPromise = nullptr;

    with_lock (NewResultWriteFileLock) {
        if (WantNewResult) {
            WantNewResult = false;
            if (Y_UNLIKELY(NewResultPromise.Defined())) {
                static const NThreading::TFuture<TStatus> errFuture
                    = NThreading::MakeFuture<TStatus>(TError{TError::EType::Logic} << "NewResultPromise has already defined (wantNewResult)");
                return errFuture;
            }

            NewResultPromise = NThreading::NewPromise<TStatus>();
            forceNewPromise = NewResultPromise.Get();
        } else if (!NewVersionFileExists && !Version.Defined() && FetchIfNotExists) {
            FetchIfNotExists = false;
            if (Y_UNLIKELY(NewResultPromise.Defined())) {
                static const NThreading::TFuture<TStatus> errFuture
                    = NThreading::MakeFuture<TStatus>(TError{TError::EType::Logic} << "NewResultPromise has already defined");
                return errFuture;
            }
            NewResultPromise = NThreading::NewPromise<TStatus>();
            forceNewPromise = NewResultPromise.Get();
        } else if (NewResultPromise.Defined()) {
            statusPromise = NewResultPromise.Get();
        }
    }

    if (forceNewPromise) {
        auto onRequestWrapper = [&onRequest, this]() {
            TStatus result = onRequest();
            if (!result.Defined()) {
                NewVersionFileExists = true;
            }
            return result;
        };
        forceNewPromise->SetValue(onRequestWrapper());
        return forceNewPromise->GetFuture();
    }

    if (statusPromise) {
        return statusPromise->GetFuture();
    }

    static const NThreading::TFuture<TStatus> success = NThreading::MakeFuture<TStatus>(Success());
    return success;
}

} // namespace NAlice::NJoker
