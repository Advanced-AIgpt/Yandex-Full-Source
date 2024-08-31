#pragma once

#include "instance_counted.h"
#include "neh.h"
#include "request.h"

#include <alice/library/util/min_heap.h>

#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/uri/uri.h>

#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/queue.h>

#include <util/system/mutex.h>

namespace NHttpFetcher {

namespace NTestSuiteNehDetail {
struct TTestCaseConnectionRetry;
struct TTestCaseConnectionRetrySmallTimeout;
struct TTestCaseWithoutConnectionRetry;
struct TTestCaseConnectionRetryFastReconnect;
struct TTestCaseConnectionRetryNormalRetries;
} // namespace NTestSuiteNehDetail

class TNehMultiFetcher;
class TNehMultiHandle;

class TOnRecv final : public NNeh::IOnRecv {
public:
    TOnRecv(TNehMultiHandle* handle, TNehMultiFetcher* multiFetcher);

    void OnNotify(NNeh::THandle& handle) override;
    void OnRecv(NNeh::THandle&) override;
    void OnEnd() override;

    void Stop();

private:
    void Release();

private:
    TNehMultiHandle* Handle;
    TNehMultiFetcher* MultiFetcher;
    bool Stopped;
    TMutex Lock;
};

struct THandleContext {
    ui32 AttemptId; // Starts from 1
    NNeh::THandleRef NehHandle;
    TNehMultiHandle* FetcherHandle;

    THandleContext(NNeh::THandleRef nehHandle, TNehMultiHandle* fetcherHandle, ui32 attempt) noexcept
        : AttemptId(attempt)
        , NehHandle(nehHandle)
        , FetcherHandle(fetcherHandle)
    {
    }
};

/**
 * Fetcher that can wait for multiple handles (TNehMultiHandle)
 * and notifies them either on response or after proper period
 * making them able to schedule something else.
 */
class TNehMultiFetcher : public TThrRefBase, public TInstanceCounted<TNehMultiFetcher> {
public:
    using TRef = TIntrusivePtr<TNehMultiFetcher>;

public:
    explicit TNehMultiFetcher(IEventLoggerPtr logger,
                              NHttpFetcher::IRequestEventListener* listener = nullptr);

    void ScheduleAttempt(TNehMultiHandle* child, NNeh::THandleRef nehHandle, TMaybe<TInstant> notifyAt, ui32 attempt);
    void ScheduleNotify(TNehMultiHandle* child, TInstant notifyAt);
    void Cancel(TNehMultiHandle* child) noexcept;

    void WaitAll(TInstant deadline);
    void CancelAll();

    void OnAttemptReady(TNehMultiHandle* child);

    IEventLoggerPtr Logger() const;

private:
    bool IsReady() const noexcept;

private:
    struct TAwakeEvent final {
        TAwakeEvent(const TInstant& when, TNehMultiHandle* handle) noexcept;

        bool IsFinalized() const noexcept;

        bool operator<(const TAwakeEvent& rhs) const noexcept {
            return When < rhs.When;
        }
        bool operator>(const TAwakeEvent& rhs) const noexcept {
            return rhs < *this;
        }

        TInstant When = {};
        TNehMultiHandle* Handle = {};
    };

    NNeh::IMultiRequesterRef RR;

    NAlice::TMinHeap<TAwakeEvent> AwakeEvents;
    TVector<THandleContext> Handles;

    bool IsCancelled;

    IRequestEventListener* EventListener;
    IEventLoggerPtr Logger_;
};

struct TNehAttemptHandle {
    TNehAttemptHandle() = default;

    TNehAttemptHandle(const TNehAttemptHandle&) = delete;
    TNehAttemptHandle(TNehAttemptHandle&&) noexcept;

    TNehAttemptHandle& operator=(const TNehAttemptHandle&) = delete;
    TNehAttemptHandle& operator=(TNehAttemptHandle&&) = delete;

    ~TNehAttemptHandle();

    TOnRecv* OnRecv = nullptr;
    NNeh::THandleRef Handle;

    ui64 RequestSize;

    TString RTLogToken;
    NRTLog::TRequestLogger* RequestLogger = {};

    void Stop();
    void Cancel();
    void Finish(bool isSuccess, TInstant finishTime = TInstant::Now(), TStringBuf errorMsg = "");

    NRTLog::TRequestLogger* GetRequestLogger();
    TInstant StartedAt;
};

/**
 * Container for http request components, adjusts headers for each attempt
 */
class TNehAttemptFactory {
public:
    TNehAttemptFactory(NUri::TUri uri, const TCgiParameters& cgi,
                       TString headers, TString body, TString contentType,
                       TProxySettingsPtr proxy, NNeh::NHttp::ERequestType reqType,
                       TString requestLabel, NRTLog::TRequestLogger* requestLogger,
                       bool overrideHttpAdapterReqId);

    TNehAttemptHandle StartAttempt(ui32 attemptId, std::unique_ptr<TOnRecv> callback);

    NNeh::TMessage CreateMessage(ui32 attemptId, TString& rtLogToken);

    const TString& GetAddress() const;

private:
    NRTLog::TRequestLogger* GetRequestLogger();

private:
    TString Uri;
    TString Headers;
    size_t HeadersSize;
    TString Body;
    TString ContentType;
    TMaybe<TString> OverrideAddr;
    NNeh::NHttp::ERequestType ReqType;
    TString RequestLabel;
    NRTLog::TRequestLogger* RequestLogger = {};
    bool OverrideHttpAdapterReqId = false;
};

/**
 * Handle for a single request adopted to TNehMultiFetcher.
 * Provides a logic for hedged retry behavior on notification (OnRetryTime()).
 */
class TNehMultiHandle : public THandle, public TInstanceCounted<TNehMultiHandle> {
public:
    TNehMultiHandle(
        TNehAttemptFactory&& nehAttemptFactory,
        const TRequestOptions& options,
        TNehMultiFetcher::TRef parent
    );
    ~TNehMultiHandle() override;

    TResponse::TRef Wait(TInstant deadline) override;

    void Cancel();

    void OnResponse(ui32 attemptId, NNeh::THandleRef response);
    void OnAwake();

    bool IsFinalized() const noexcept;

private:
    void MakeRetryAttempt(TInstant now);
    void DoMakeAttempt(TInstant now, bool connectionRetry);

    void FailByTimeout(TDuration duration);

    void CancelAttempts();

private:
    TNehAttemptFactory NehAttemptFactory;
    const TRequestOptions Options;

    const TInstant StartedAt;
    const TInstant Deadline;

    TNehMultiFetcher::TRef Parent;
    TVector<TNehAttemptHandle> NehActiveAttempts;
    THashSet<NNeh::THandleRef> RetiredHandles;

    bool FastReconnectAllowed;

    ui32 AttemptsCount;
    TMaybe<TInstant> NextRetry;

    TResponse::TRef FinalResponse;

    TResponse::TRef LastError;
    TMaybe<TInstant> LastErrorFinishTime;
    TMaybe<TFetchStatus> LastErrorStatus;

    ui32 ConnectAttempsCount;
    TInstant LastConnectAttemptTime;

    friend struct NTestSuiteNehDetail::TTestCaseConnectionRetry;
    friend struct NTestSuiteNehDetail::TTestCaseConnectionRetrySmallTimeout;
    friend struct NTestSuiteNehDetail::TTestCaseWithoutConnectionRetry;
    friend struct NTestSuiteNehDetail::TTestCaseConnectionRetryFastReconnect;
    friend struct NTestSuiteNehDetail::TTestCaseConnectionRetryNormalRetries;
};


class TNehRequest : public TRequest {
public:
    TNehRequest(NUri::TUri uri, TProxySettingsPtr proxyOverride,
                NRTLog::TRequestLogger* requestLogger, bool overrideHttpAdapterReqId);

    // TRequest overrides.
    TRequest& AddHeader(TStringBuf key, TStringBuf value) override;
    TVector<TString> GetHeaders() const override;
    bool HasHeader(TStringBuf name) const override;
    const TString& GetBody() const override;
    void SetPath(TStringBuf path) override;
    TString GetMethod() const override;
    TRequest& SetProxy(const TString& proxy) override;
    TRequest& SetMethod(TStringBuf method) override;
    TRequest& SetBody(TStringBuf body, TStringBuf method) override;
    TString Url() const override;
    TRequest& SetContentType(TStringBuf value) override;

protected:
    TNehAttemptFactory MakeNehAttemptFactory();

private:
    static NUri::TUri ConstructUriAndCgi(NUri::TUri uri, TCgiParameters& cgi);

private:
    TString Headers;
    NUri::TUri Uri;
    TString Body;
    TString ContentType;
    TMaybe<TString> ForceProxy;
    TProxySettingsPtr ProxyOverride;
    NNeh::NHttp::ERequestType Type;
    NRTLog::TRequestLogger* RequestLogger = nullptr;
    bool OverrideHttpAdapterReqId = false;
};

TRequestPtr SingleNehRequest(NUri::TUri uri, TProxySettingsPtr proxyOverride = {}, NRTLog::TRequestLogger* requestLogger = {});
TRequestPtr RetryableNehRequest(NUri::TUri uri, const TRequestOptions& options, IEventLoggerPtr logger,
                                NHttpFetcher::IRequestEventListener* listener);
TRequestPtr AttachedNehRequest(NUri::TUri uri, const TRequestOptions& options, TNehMultiFetcher::TRef multiFetcher);

} // namespace NHttpFetcher
