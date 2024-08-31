#pragma once

#include "session.h"

#include <alice/joker/library/session/run_log_writer.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/flags.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/rwlock.h>

namespace NAlice::NJoker {

class TGlobalContext;
class THttpContext;

class THttpProxySession final : public TSession {
public:
    struct TStubMeta : public TThrRefBase {
        TStubMeta(const THttpProxySession& session, const TStubId& stubId, TMaybe<TString> version = Nothing());

        NThreading::TFuture<TStatus> RemoteRequestIfNeeded(std::function<TStatus()> onRequest);

        TMaybe<TString> Version;

        bool NewVersionFileExists;

        TMutex NewResultWriteFileLock;
        bool WantNewResult = false;
        bool FetchIfNotExists = true;
        bool ImitateDelay = false;
        using TNewResultPromise = NThreading::TPromise<TStatus>;
        TMaybe<TNewResultPromise> NewResultPromise;
    };
    using TStubMetaPtr = TIntrusivePtr<TStubMeta>;

public:
    enum class EFlag {
        FetchIfNotExists = 1ULL << 0 /* "fetch_if_not_exists" */,
        ForceUpdate      = 1ULL << 1 /* "force_update" */,
        ImitateDelay     = 1ULL << 2 /* "imitate_delay" */,
    };
    Y_DECLARE_FLAGS(TFlags, EFlag);

public:
    static THttpProxySession* GetSession(const TSessionId& id);
    static THttpProxySession* NewSession(const TSessionId& id, TGlobalContext& globalCtx, TFlags flags);

public:
    THttpProxySession(TSessionId sessionId, TGlobalContext& globalCtx, TFlags flags);

    /** Get stub data by stubRequest and output it via httpCtx.
     */
    TStatus ReplyTo(THttpContext& httpCtx, const TStubId& stubId);

    /** Get stub data from backend and cache/save it in working dir.
     */
    TStatus SyncVersionedStub(const TStubId& stubId, const TString& version);

    // FIXME move to the base class.
    TStatus SaveStub(const TStubId& id, const TString* version, TStubItemPtr stubItem) const;

    TStatus ObtainViaRemoteRequest(const TStubId& stubId, TDuration timeout, THttpContext& httpCtx, TStubItemPtr& stub) const;

private:
    TStubMetaPtr LoadOrCreateStubMeta(const TStubId& stubId);

private:
    class TStubWrapper {
    public:
        TStubWrapper(const TStubId& stubId, TStubMeta& stubMeta);

        const TStubId& StubId() const {
            return StubId_;
        }

        TStubMeta& Meta() {
            return StubMeta_;
        }

    private:
        const TStubId& StubId_;
        TStubMeta& StubMeta_;
    };

    TErrorOr<TStubItemPtr> ProcessRequest(THttpContext& httpCtx, TStubWrapper& stub);

private:
    TGlobalContext& GlobalCtx_;
    TFlags Flags_;
    TRunLogWriter RunLogWriter_;

    // request_uniq_hash => stub
    THashMap<TString, TStubMetaPtr> Stubs_;
    TMutex StubsLock_;

private:
    static THashMap<TString, THttpProxySession> Sessions_;
    static TRWMutex SessionsLock_;
};

Y_DECLARE_OPERATORS_FOR_FLAGS(THttpProxySession::TFlags)

} // namespace NAlice::NJoker
