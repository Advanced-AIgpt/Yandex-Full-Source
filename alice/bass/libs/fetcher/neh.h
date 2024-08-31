#pragma once

#include "event_logger.h"
#include "request.h"

#include <alice/library/network/common.h>

#include <library/cpp/uri/uri.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

namespace NRTLog {
class TRequestLogger;
} // namespace NRTLog

namespace NHttpFetcher {

struct TFetchStatus {
    enum class EStatus {
        StatusSuccess,
        StatusError,
        StatusFailure
    };

    static const TString HTTP_0;
    static const TString HTTP_1XX;
    static const TString HTTP_2XX;
    static const TString HTTP_3XX;
    static const TString HTTP_4XX;
    static const TString HTTP_5XX;

    EStatus Status;
    TString Reason;

    static TFetchStatus Success();
    static TFetchStatus Error(ui32 httpCode);
    static TFetchStatus Error(TString reason = "");
    static TFetchStatus Failure(ui32 httpCode);
    static TFetchStatus Failure(TString reason = "");

    bool IsSuccess() const;
    bool IsError() const;
    bool IsFailure() const;

    static TString ClassifyHttpCode(ui32 code);
};

struct TRequestStats {
    ui64 RequestSize = 0;
};

struct TResponseStats {
    TResponseStats(const TResponse& response, const TFetchStatus& status, TDuration slaTime, TDuration duration)
        : Status(status)
        , Duration(duration)
        , SlaTime(slaTime)
        , ResponseSize(response.Data.size())
        , Result(response.Result)
        , HttpCode(response.Code)
    {
    }

    TResponseStats(const TResponse& response, const TFetchStatus& status, TDuration slaTime)
        : TResponseStats(response, status, slaTime, response.Duration)
    {
    }

    TFetchStatus Status;
    TDuration Duration;
    TDuration SlaTime;
    ui64 ResponseSize;
    TResponse::EResult Result;
    TResponse::THttpCode HttpCode;
};

class IMetrics : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<IMetrics>;

    virtual ~IMetrics() = default;

    virtual void OnStartRequest() = 0;
    virtual void OnFinishRequest(const TResponseStats& stats) = 0;

    virtual void OnStartHedgedRequest(const TRequestStats& stats) = 0;
    virtual void OnFinishHedgedRequest(const TResponseStats& stats) = 0;
};

class IStatusCallback : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<IStatusCallback>;

    virtual ~IStatusCallback() = default;
    virtual TFetchStatus OnResponse(const TResponse&) = 0;
};
IStatusCallback::TRef DefaultStatusCallback();


class IRequestEventListener : public TThrRefBase {
public:
    using TCallback = std::function<void(NHttpFetcher::THandle::TRef)>;

public:
    virtual ~IRequestEventListener() = default;
    virtual void OnAttempt(THandle::TRef handle) = 0;
    virtual void AddHandleObserver(THandle::TRef handle, TCallback callback) = 0;
};

struct TRequestOptions {
    TDuration RetryPeriod = TDuration::Zero();
    TDuration Timeout = TDuration::Max();

    ui32 MaxAttempts = 1;

    /**
     * in MultiRequest, if current request fails, cancel active requests to other sources
     * NOTE: already successful requests are kept untouched
     */
    bool IsRequired = true;

    /**
     * Workaround over connection-reset-by-peer problems with no response when session died before keep-alive timeouts.
     * Only for sources with MaxAttempts == 1 (not needed for services with normal retries enabled).
     * Useful when retries are normally NOT allowed for a source, but another connection attempt could solve the problem.
     * WARNING! Use it with care: all attempts (even failed ones) may actually reach the source with this option.
     *          Do NOT enable it if the source does not allow retries at all.
     */
    bool EnableFastReconnect = false;

    TDuration FastReconnectLimit = TDuration::MilliSeconds(50); // Period from request start when fast-reconnect is still allowed.

    IMetrics::TRef Metrics;
    TDuration SLATime = Timeout;
    IStatusCallback::TRef StatusCallback;
    std::function<void(const TRequest&)> BeforeFetchCallback = [] (const TRequest&) {};

    /** Proxy settings object allows to pass request via specified proxy and add some headers.
     */
    TProxySettingsPtr ProxyOverride;

    NRTLog::TRequestLogger* RequestLogger = nullptr;

    bool LogErrors = false;

    bool OverrideHttpAdapterReqId = false;

    ui32 MaxConnectionAttempts = 1;
    TDuration MaxConnectionAttemptMs = TDuration::MilliSeconds(50);
    TStringBuf Name;
};

/**
 * Multiple request factory and waiter.
 *
 * 1. Create a request via ::AddRequest and provide additional parameters via TRequest interface
 * 2. Call TRequest() to obtain THandle for a request
 * 3. Call WaitAll() to initiate multiple requests waiting mechanism.
 *    Each request will be retried and post-processed according to provided TRequestOptions.
 *    If one of required (options.IsRequired == true) request fails (TFetchStatus::IsFailure()), all waited responses are abandoned and waiting requests will fail.
 *    If some lucky request already got a response at that moment, it will be left in a success state with a valid answer, though.
 */
class IMultiRequest : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<IMultiRequest>;

    virtual ~IMultiRequest() = default;
    virtual THolder<TRequest> AddRequest(const NUri::TUri& uri, const TRequestOptions& options) = 0;
    virtual void WaitAll(TInstant deadline) = 0;

    THolder<TRequest> AddRequest(TStringBuf url, const TRequestOptions& options);

    void WaitAll() {
        WaitAll(TInstant::Max());
    }
};

inline NUri::TUri ParseUri(TStringBuf url) {
    return NAlice::NNetwork::ParseUri(url);
}

TStringBuf ClassifyHttpCode(ui32 code); //TODO: drop all external calls and remove it from interface

class IRequestAPI {
public:
    virtual ~IRequestAPI() = default;
    virtual TRequestPtr Request(const NUri::TUri& uri, const TRequestOptions& options,
                                IRequestEventListener* listener) = 0;
    virtual TRequestPtr Request(TStringBuf url, const TRequestOptions& options, IRequestEventListener* listener) = 0;
    virtual IMultiRequest::TRef MultiRequest(IRequestEventListener* listener = nullptr) = 0;
    virtual IMultiRequest::TRef WeakMultiRequest(IRequestEventListener* listener = nullptr) = 0;
};

class TRequestAPI : public IRequestAPI {
public:
    explicit TRequestAPI(IEventLoggerPtr logger)
        : Logger(logger) {
    }

    virtual ~TRequestAPI() = default;

    TRequestPtr Request(const NUri::TUri& uri, const TRequestOptions& options,
                        IRequestEventListener* listener = nullptr) override;
    TRequestPtr Request(TStringBuf url, const TRequestOptions& options,
                        IRequestEventListener* listener = nullptr) override;
    IMultiRequest::TRef MultiRequest(IRequestEventListener* listener = nullptr) override;
    IMultiRequest::TRef WeakMultiRequest(IRequestEventListener* listener = nullptr) override;

private:
    IEventLoggerPtr Logger;
};


inline TRequestAPI BassRequestAPI() {
    return TRequestAPI{MakeIntrusive<TBassEventLogger>()};
}

// Following functions are deprecated, please, don't use them. Use
// TRequestAPI instead.
TRequestPtr Request(const NUri::TUri& uri, const TRequestOptions& options, IRequestEventListener* listener = nullptr);
TRequestPtr Request(TStringBuf url, const TRequestOptions& options, IRequestEventListener* listener = nullptr);
IMultiRequest::TRef MultiRequest();
IMultiRequest::TRef WeakMultiRequest();

} // namespace NHttpFetcher
