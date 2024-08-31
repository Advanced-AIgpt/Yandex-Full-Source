#include "neh_detail.h"

#include "util.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/network/common.h>
#include <alice/library/network/headers.h>
#include <alice/rtlog/client/client.h>

#include <library/cpp/http/io/stream.h>
#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/neh/http_common.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/network/socket.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/split.h>

using namespace NAlice;

namespace NHttpFetcher {
namespace {

const THashSet<TResponse::TSystemErrorCode> CONNECTION_ERRORS = {
    101, // Network is unreachable
    110, // Connection timed out
    111, // Connection refused
    112, // Host is down
    113  // No route to host
};

class TErrorHandle : public THandle {
public:
    TErrorHandle(TResponse::EResult result, const TDuration& duration, TStringBuf errMsg) {
        Response = MakeIntrusive<TResponse>(result, duration, errMsg, /* systemErrorCode= */ 0);
    }

    TResponse::TRef Wait(TInstant /* deadline */) override {
        return Response;
    }
};

TResponse::TRef ResponseFromNeh(NNeh::TResponseRef r, TDuration fallbackTimeout = TDuration::Zero()) {
    if (!r)
        return MakeIntrusive<TResponse>(TResponse::EResult::Timeout, fallbackTimeout, TStringBuf("no response"),
                                        /* systemErrorCode= */ 0);
    if (r->IsError()) {
        switch (r->GetErrorType()) {
            case NNeh::TError::TType::ProtocolSpecific:
                return MakeIntrusive<TResponse>(r->GetErrorCode(), std::move(r->Data), r->Duration, r->GetErrorText(),
                                                std::move(r->Headers));
            case NNeh::TError::TType::Cancelled:
                return MakeIntrusive<TResponse>(TResponse::EResult::Timeout, r->Duration, r->GetErrorText(),
                                                r->GetSystemErrorCode());
            case NNeh::TError::TType::UnknownType:
            default:
                return MakeIntrusive<TResponse>(TResponse::EResult::EmptyResponse, r->Duration, r->GetErrorText(), r->GetSystemErrorCode());
        }
    } else { // success
        const auto rawCode = ParseHttpRetCode(r->FirstLine);
        const auto code = IsHttpCode(rawCode) ? static_cast<HttpCodes>(rawCode) : HTTP_OK;
        return MakeIntrusive<TResponse>(static_cast<TResponse::THttpCode>(code), std::move(r->Data), r->Duration,
                                        r->GetErrorText(), std::move(r->Headers));
    }
}

class TNehMultiRequest : public TNehRequest {
private:
    TRequestOptions Options;
    TNehMultiFetcher::TRef MultiFetcher;

public:
    TNehMultiRequest(NUri::TUri uri, const TRequestOptions& options, TNehMultiFetcher::TRef multiFetcher)
        : TNehRequest(uri, options.ProxyOverride, options.RequestLogger, options.OverrideHttpAdapterReqId)
        , Options(options)
        , MultiFetcher(multiFetcher)
    {
        if (!Options.StatusCallback) {
            Options.StatusCallback = DefaultStatusCallback();
        }
        AddHeader(TStringBuf("Request-Timeout"), ToString(options.Timeout.MicroSeconds()));
    }

    THandle::TRef Fetch() override {
        try {
            Options.BeforeFetchCallback(*this);
            return new TNehMultiHandle(MakeNehAttemptFactory(), Options, MultiFetcher);
        } catch (const TNetworkResolutionError&) {
            return MakeIntrusive<TErrorHandle>(TResponse::EResult::NetworkResolutionError, TDuration{},
                                               CurrentExceptionMessage());
        }
    }
};

class TNehSingleHandle: public THandle {
public:
    explicit TNehSingleHandle(TNehAttemptFactory&& nehAttemptFactory)
        : NehAttemptHandle(nehAttemptFactory.StartAttempt(/* attemptId= */ 0, {}))
        , StartedAt(TInstant::Now())
    {
    }

    TResponse::TRef Wait(TInstant deadline) override {
        NNeh::TResponseRef r;
        if (!NehAttemptHandle.Handle->Wait(r, deadline) || !r) {
            NehAttemptHandle.Cancel();
        } else {
            NehAttemptHandle.Finish(!r->IsError(), StartedAt + r->Duration, r->GetErrorText());
        }
        Response = ResponseFromNeh(r, Now() - StartedAt);
        return Response;
    }

private:
    TNehAttemptHandle NehAttemptHandle;
    const TInstant StartedAt;
};

class TNehSingleRequest : public TNehRequest {
public:
    TNehSingleRequest(NUri::TUri uri,
                      TProxySettingsPtr proxyOverride,
                      NRTLog::TRequestLogger* requestLogger,
                      bool overrideHttpAdapterReqId = false)
        : TNehRequest(uri, proxyOverride, requestLogger, overrideHttpAdapterReqId)
    {
    }

    THandle::TRef Fetch() override {
        try {
            return new TNehSingleHandle(MakeNehAttemptFactory());
        } catch (const TNetworkResolutionError& e) {
            return MakeIntrusive<TErrorHandle>(TResponse::EResult::NetworkResolutionError, TDuration{},
                                               CurrentExceptionMessage());
        }
    }
};

} // namespace

// TNehMultiFetcher -----------------------------------------------------------
TNehMultiFetcher::TNehMultiFetcher(IEventLoggerPtr logger, NHttpFetcher::IRequestEventListener* listener)
    : RR(NNeh::CreateRequester())
    , IsCancelled(false)
    , EventListener{listener}
    , Logger_(logger)
{
    Y_ASSERT(Logger_);
}

void TNehMultiFetcher::ScheduleAttempt(TNehMultiHandle* child, NNeh::THandleRef nehHandle, TMaybe<TInstant> notifyAt, ui32 attempt) {
    if (IsCancelled) {
        nehHandle->Cancel();
        nehHandle->Wait();
        child->OnResponse(attempt, nehHandle);
        return;
    }

    RR->Add(nehHandle);
    Handles.emplace_back(nehHandle, child, attempt);

    if (notifyAt) {
        ScheduleNotify(child, *notifyAt);
    }
}

void TNehMultiFetcher::ScheduleNotify(TNehMultiHandle* child, TInstant notifyAt) {
    AwakeEvents.Emplace(notifyAt, child);
}

void TNehMultiFetcher::Cancel(TNehMultiHandle* child) noexcept {
    EraseIf(Handles, [&](const THandleContext& hc) { return hc.FetcherHandle == child; });

    for (auto& event : AwakeEvents) {
        if (event.Handle == child) {
            event.Handle = nullptr;
        }
    }
}

void TNehMultiFetcher::WaitAll(TInstant deadline) {
    while (!IsReady()) {
        // Cleanup all finalized events
        while (!AwakeEvents.Empty() && AwakeEvents.GetMin().IsFinalized()) {
            AwakeEvents.ExtractMin();
        }
        TInstant awakeAt = AwakeEvents.Empty() ? TInstant::Max() : AwakeEvents.GetMin().When;

        /**
         * Wait until:
         * (a) One of scheduled requests receives it's response, it can be accessed via nehHandle variable:
         *     NNeh::IMultiRequester::Wait() will set it, see [1] for more info
         * or
         * (b) The moment of awakeAt occurs: Wait() will return false
         *
         * Response is passed to it's corresponding TNehMultiHandle via OnResponse() callback,
         * so it will have a chance to schedule a retry or do something else.
         *
         * [1] https://wiki.yandex-team.ru/development/poisk/arcadia/library/cpp/neh/#vypolnenieneskolkixparallelnyxzaprosov
         */
        NNeh::THandleRef nehHandle;
        while (!IsReady() && RR->Wait(nehHandle, Min(awakeAt, deadline))) {
            THandleContext* ptr = FindIfPtr(Handles, [&nehHandle] (const THandleContext& handles) {
                return handles.NehHandle.Get() == nehHandle.Get();
            });
            if (ptr) {
                ptr->FetcherHandle->OnResponse(ptr->AttemptId, nehHandle);
            } else {
                Logger_->OnAbandonedRequest(GetInstanceId());
            }
        }
        if (awakeAt >= deadline) {
            return;
        }

        while (!AwakeEvents.Empty() && AwakeEvents.GetMin().When <= awakeAt) {
            const auto& top = AwakeEvents.GetMin();
            if (!top.IsFinalized()) {
                Y_ASSERT(top.Handle);
                top.Handle->OnAwake();
            }
            AwakeEvents.ExtractMin();
        }
    }
}

void TNehMultiFetcher::CancelAll() {
    for (THandleContext& hc : Handles) {
        if (!hc.FetcherHandle->IsFinalized()) {
            hc.FetcherHandle->Cancel();
        }
    }
    AwakeEvents.Clear();
    IsCancelled = true;
}

IEventLoggerPtr TNehMultiFetcher::Logger() const {
    return Logger_;
}

void TNehMultiFetcher::OnAttemptReady(TNehMultiHandle* child) {
    if (EventListener) {
        EventListener->OnAttempt(child);
    }
}

bool TNehMultiFetcher::IsReady() const noexcept {
    return AllOf(Handles, [] (const THandleContext& hc) { return hc.FetcherHandle->IsFinalized(); });
}

TNehMultiFetcher::TAwakeEvent::TAwakeEvent(const TInstant& when, TNehMultiHandle* handle) noexcept
    : When(when)
    , Handle(handle)
{
}

bool TNehMultiFetcher::TAwakeEvent::IsFinalized() const noexcept {
    return !Handle || Handle->IsFinalized();
}

TNehAttemptFactory::TNehAttemptFactory(NUri::TUri uri, const TCgiParameters& cgi,
                                       TString headers, TString body, TString contentType,
                                       TProxySettingsPtr proxy, NNeh::NHttp::ERequestType reqType, TString requestLabel,
                                       NRTLog::TRequestLogger* requestLogger, bool overrideHttpAdapterReqId)
    : Uri(UrlWithCgiParams(std::move(uri), cgi))
    , Headers(std::move(headers))
    , HeadersSize(Headers.size())
    , Body(std::move(body))
    , ContentType(std::move(contentType))
    , ReqType(reqType)
    , RequestLabel(std::move(requestLabel))
    , RequestLogger(requestLogger)
    , OverrideHttpAdapterReqId(overrideHttpAdapterReqId)
{
    if (proxy) {
        proxy->PrepareRequest(uri, cgi, Uri, Headers, OverrideAddr);
        HeadersSize = Headers.size();
    }
}


NNeh::TMessage TNehAttemptFactory::CreateMessage(ui32 attemptId, TString& rtLogToken) {
    Headers.resize(HeadersSize);

    if (Headers) {
        Headers.append(CRLF);
    }
    Headers.append(TStringBuf("X-Fetcher-Attempt-Number: "));
    Headers.append(ToString(attemptId));

    NRTLog::TRequestLogger* requestLogger = GetRequestLogger();
    if (requestLogger && RequestLabel) {
        rtLogToken = requestLogger->LogChildActivationStarted(false, RequestLabel);
        if (Headers) {
            Headers.append(CRLF);
        }
        Headers.append(NAlice::NNetwork::HEADER_X_RTLOG_TOKEN);
        Headers.append(TStringBuf(": "));
        Headers.append(rtLogToken);

        if (OverrideHttpAdapterReqId) {
            Headers.append(CRLF);
            Headers.append(TStringBuilder() << NAlice::NNetwork::HEADER_X_YANDEX_REQ_ID << ": ");
            Headers.append(rtLogToken);
        }
    }

    NNeh::NHttp::ERequestFlags requestFlags;
    if (OverrideAddr.Defined()) {
        requestFlags |= NNeh::NHttp::ERequestFlag::AbsoluteUri;
    }

    NNeh::TMessage msg(NNeh::TMessage::FromString(Uri));
    if (!NNeh::NHttp::MakeFullRequest(msg, Headers, Body, ContentType, ReqType, requestFlags)) {
        ythrow yexception() << "Unable to prepare neh request: '" << msg.Addr << "', '" << Uri << '"';
    }

    if (OverrideAddr.Defined()) {
        msg.Addr = TString::Join(TStringBuf{"full://"}, *OverrideAddr);
    }

    return msg;
}

TNehAttemptHandle TNehAttemptFactory::StartAttempt(const ui32 attemptId, std::unique_ptr<TOnRecv> callback) {
    TNehAttemptHandle result;

    auto msg = CreateMessage(attemptId, result.RTLogToken);
    result.StartedAt = Now();
    result.Handle = NNeh::Request(msg, callback.get());
    result.OnRecv = callback.release();
    result.RequestLogger = GetRequestLogger();
    result.RequestSize = msg.Data.size();
    return result;
}

const TString& TNehAttemptFactory::GetAddress() const {
    return Uri;
}

NRTLog::TRequestLogger* TNehAttemptFactory::GetRequestLogger() {
    return RequestLogger ? RequestLogger : TLogging::RequestLogger.Get();
}

void TNehAttemptHandle::Stop() {
    if (OnRecv) {
        OnRecv->Stop();
        OnRecv = nullptr;
    }
}

void TNehAttemptHandle::Cancel() {
    // Cancel() can be called from TNehMultiHandle dtor before attempt handles are destroyed.
    // Notifying a listener about a cancellation of a handle being destroyed is a potential UAF
    // so we stop the callback.
    Stop();
    Handle->Cancel();
    Finish(false);
}

NRTLog::TRequestLogger* TNehAttemptHandle::GetRequestLogger() {
    return RequestLogger ? RequestLogger : TLogging::RequestLogger.Get();
}

TNehAttemptHandle::TNehAttemptHandle(TNehAttemptHandle&& rhs) noexcept
    : OnRecv(rhs.OnRecv)
    , Handle{std::move(rhs.Handle)}
    , RequestSize{rhs.RequestSize}
    , RTLogToken{std::move(rhs.RTLogToken)}
    , RequestLogger{rhs.RequestLogger}
    , StartedAt{rhs.StartedAt}
{
    rhs.OnRecv = nullptr;
}

TNehAttemptHandle::~TNehAttemptHandle() {
    Stop();
}

void TNehAttemptHandle::Finish(bool isSuccess, TInstant finishTime, TStringBuf errorMsg) {
    NRTLog::TRequestLogger* requestLogger = GetRequestLogger();
    if (!Handle || !RTLogToken || !requestLogger) {
        return;
    }
    requestLogger->LogChildActivationFinished(finishTime.MicroSeconds(), RTLogToken, isSuccess, errorMsg);
    RTLogToken.clear();
}

TNehMultiHandle::TNehMultiHandle(
    TNehAttemptFactory&& nehAttemptFactory,
    const TRequestOptions& options,
    TNehMultiFetcher::TRef parent
)
    : NehAttemptFactory(std::move(nehAttemptFactory))
    , Options(options)
    , StartedAt(Now())
    , Deadline(StartedAt + Options.Timeout)
    , Parent(parent)
    , FastReconnectAllowed(options.EnableFastReconnect)
    , AttemptsCount(0)
    , ConnectAttempsCount(0)
    , LastConnectAttemptTime(StartedAt)
{
    Y_ASSERT(Parent);
    Parent->Logger()->OnAttemptRegistered({GetInstanceId(), Options.Name}, Parent->GetInstanceId(), NehAttemptFactory.GetAddress());

    if (Options.Metrics) {
        Options.Metrics->OnStartRequest();
    }
    try {
        Parent->ScheduleNotify(this, Deadline);
        DoMakeAttempt(StartedAt, /* connectionRetry= */ false);
    } catch (...) {
        Parent->Cancel(this);
        throw;
    }
}

TNehMultiHandle::~TNehMultiHandle() {
    CancelAttempts();
    Parent->Cancel(this);
}

TResponse::TRef TNehMultiHandle::Wait(TInstant deadline) {
    if (FinalResponse) {
        return FinalResponse;
    }
    TInstant waitTill = Min(Deadline, deadline);
    Parent->WaitAll(waitTill);

    if (!FinalResponse) {
        TMaybe<TFetchStatus> finalStatus;
        TDuration duration = waitTill - StartedAt;

        if (LastError) {
            finalStatus = TFetchStatus::Failure(LastErrorStatus->Reason);
        } else {
            finalStatus = TFetchStatus::Failure("noResponse");
            FailByTimeout(duration);
        }
        FinalResponse = LastError;
        if (Options.Metrics) {
            Options.Metrics->OnFinishRequest(TResponseStats(*FinalResponse, *finalStatus, Options.SLATime, duration));
        }
    }
    return FinalResponse;
}

void TNehMultiHandle::Cancel() {
    if (FinalResponse) {
        return;
    }
    CancelAttempts();
    FinalResponse = MakeIntrusive<TResponse>(TResponse::EResult::EmptyResponse, Now() - StartedAt,
                                             TStringBuf("cancelled"), /* systemErrorCode= */ 0);
}

void TNehMultiHandle::MakeRetryAttempt(TInstant now) {
    if (AttemptsCount < Options.MaxAttempts) {
        DoMakeAttempt(now, /* connectionRetry= */ false);
    }
}

void TNehMultiHandle::DoMakeAttempt(TInstant now, bool connectionRetry) {
    if (FinalResponse) {
        return;
    }
    if (!connectionRetry) {
        ++AttemptsCount;
    } else {
        ++ConnectAttempsCount;
    }
    ui32 attemptId = AttemptsCount + ConnectAttempsCount;
    Parent->Logger()->OnAttemptStarted({GetInstanceId(), Options.Name}, attemptId, Options.MaxAttempts);

    auto onRecv = std::make_unique<TOnRecv>(this, Parent.Get());
    auto attempt = NehAttemptFactory.StartAttempt(attemptId, std::move(onRecv));
    if (Options.Metrics) {
        Options.Metrics->OnStartHedgedRequest(TRequestStats{.RequestSize = attempt.RequestSize});
    }
    auto handle = attempt.Handle;
    NehActiveAttempts.push_back(std::move(attempt));

    if (!connectionRetry && AttemptsCount < Options.MaxAttempts) {
        NextRetry = now + Options.RetryPeriod;
    } else if (connectionRetry && ConnectAttempsCount + 1 < Options.MaxConnectionAttempts) {
        NextRetry = now;
    } else {
        NextRetry.Clear();
    }
    Parent->ScheduleAttempt(this, handle, NextRetry, attemptId);
}

void TNehMultiHandle::FailByTimeout(TDuration duration) {
    CancelAttempts();
    LastError = MakeIntrusive<TResponse>(TResponse::EResult::Timeout, duration, TStringBuf("no response"),
                                         /* systemErrorCode= */ 0);
}

void TNehMultiHandle::CancelAttempts() {
    for (auto& a: NehActiveAttempts) {
        a.Cancel();
    }
}

namespace {
bool HttpCodeMeansRespectRetryPeriod(ui32 code) {
    return code == HTTP_REQUEST_TIME_OUT ||
        code == HTTP_TOO_MANY_REQUESTS ||

        code == HTTP_SERVICE_UNAVAILABLE ||
        code == HTTP_BANDWIDTH_LIMIT_EXCEEDED;
}
} // namespace anonymous

TOnRecv::TOnRecv(TNehMultiHandle* handle, TNehMultiFetcher* multiFetcher)
    : Handle{handle}
    , MultiFetcher{multiFetcher}
    , Stopped{false}
{
}

void TOnRecv::OnNotify(NNeh::THandle&) {
    with_lock (Lock) {
        if (!Stopped) {
            Stopped = true;
            MultiFetcher->OnAttemptReady(Handle);
        }
    }
}

/// Prevent a situation where a neh handle is still kept (by neh) but fetcher structures are already destroyed.
void TOnRecv::Stop() {
    with_lock (Lock) {
        Stopped = true;
    }
}

void TOnRecv::OnRecv(NNeh::THandle&) {
    Release();
}

void TOnRecv::OnEnd() {
    Release();
}

void TOnRecv::Release() {
    // NOTE: The neh handle manages the callback lifetime by calling OnEnd() and OnRecv() when it is no longer needed.
    delete this;
}

void TNehMultiHandle::OnResponse(ui32 attemptId, NNeh::THandleRef nehHandle) {
    if (FinalResponse) {
        return;
    }

    const auto nehResponse = nehHandle->Get();
    TInstant now = Now();
    TDuration fallbackDuration = now - StartedAt;
    TMaybe<TDuration> responseDuration = nehResponse ? MakeMaybe(nehResponse->Duration) : Nothing();
    TResponse::TRef response = ResponseFromNeh(nehResponse, fallbackDuration);

    const TFetchStatus status = Options.StatusCallback->OnResponse(*response);
    if (Options.Metrics) {
        Options.Metrics->OnFinishHedgedRequest(TResponseStats{*response, status, Options.SLATime});
    }

    TDuration duration = fallbackDuration;
    {
        auto* attempt = FindIfPtr(NehActiveAttempts, [&nehHandle](const auto& r) {
            return r.Handle.Get() == nehHandle.Get();
        });
        Y_ENSURE(attempt);
        response->RTLogToken = attempt->RTLogToken;
        if (responseDuration) {
            duration = attempt->StartedAt - StartedAt + *responseDuration;
        }
        attempt->Finish(status.IsSuccess(), StartedAt + duration, response->GetErrorText());
    }

    if (!status.IsSuccess() && Options.LogErrors) {
        Parent->Logger()->OnAttemptError({GetInstanceId(), Options.Name}, attemptId, response->Data);
    }

    if (status.IsSuccess() || status.IsFailure()) {
        FinalResponse = response;
        if (status.IsFailure()) {
            FinalResponse->Result = TResponse::EResult::DataError;
            if (Options.IsRequired) {
                Parent->CancelAll();
            }
        }

        if (Options.Metrics) {
            Options.Metrics->OnFinishRequest(TResponseStats{*response, status, Options.SLATime});
        }
        Parent->Logger()->OnAttemptSuccess({GetInstanceId(), Options.Name}, attemptId, duration);
        return;
    }
    Parent->Logger()->OnAttemptError({GetInstanceId(), Options.Name}, attemptId, duration, response->GetErrorText());

    LastError = response;
    LastErrorStatus = status;
    LastErrorFinishTime = StartedAt + duration;

    // Do not schedule retry immediately if HTTP code means it is useless
    if (nehResponse && HttpCodeMeansRespectRetryPeriod(nehResponse->GetErrorCode())) {
        return;
    }
    // Do not retry cancelled request
    if (nehResponse && nehResponse->GetErrorType() == NNeh::TError::Cancelled) {
        return;
    }

    if (!nehResponse && AttemptsCount == 1 && Options.MaxAttempts == 1) {
        if (FastReconnectAllowed && (now - StartedAt) < Options.FastReconnectLimit) {
            FastReconnectAllowed = false;
            DoMakeAttempt(now, /* connectionRetry= */ false);
        } else if (CONNECTION_ERRORS.contains(response->GetSystemErrorCode()) &&
                   ConnectAttempsCount + 1 < Options.MaxConnectionAttempts &&
                   (now - LastConnectAttemptTime) < Options.MaxConnectionAttemptMs) {
            LastConnectAttemptTime = Now();
            DoMakeAttempt(now, /* connectionRetry= */ true);
        }
    }

    // Retry now in all other cases
    MakeRetryAttempt(now);
}

void TNehMultiHandle::OnAwake() {
    TInstant now = Now();
    if ((LastErrorFinishTime.Defined() ? *LastErrorFinishTime : now) >= Deadline) {
        FailByTimeout(Deadline - StartedAt);
        return;
    }
    if (!NextRetry || now < NextRetry) {
        /**
         * If retry was not scheduled or was rescheduled later, then
         * do nothing right now.  This could happen if we got response
         * with retryable error before: the retry has already happen
         * then so next retry must occur only after RetryPeriod.
         */
        return;
    }
    MakeRetryAttempt(now);
}

bool TNehMultiHandle::IsFinalized() const noexcept {
    return !!FinalResponse;
}

TRequestPtr SingleNehRequest(NUri::TUri uri, TProxySettingsPtr proxyOverride, NRTLog::TRequestLogger* requestLogger) {
    return MakeHolder<TNehSingleRequest>(std::move(uri), proxyOverride, requestLogger);
}

TRequestPtr RetryableNehRequest(NUri::TUri uri, const TRequestOptions& options, IEventLoggerPtr logger,
                                NHttpFetcher::IRequestEventListener* listener) {
    TNehMultiFetcher::TRef multiFetcher = new TNehMultiFetcher(logger, listener);
    return MakeHolder<TNehMultiRequest>(std::move(uri), options, multiFetcher);
}

TRequestPtr AttachedNehRequest(NUri::TUri uri, const TRequestOptions& options, TNehMultiFetcher::TRef multiFetcher) {
    return MakeHolder<TNehMultiRequest>(std::move(uri), options, multiFetcher);
}

// TNehRequest ----------------------------------------------------------------
TNehRequest::TNehRequest(NUri::TUri uri, TProxySettingsPtr proxyOverride,
                         NRTLog::TRequestLogger* requestLogger, bool overrideHttpAdapterReqId)
    : Uri(ConstructUriAndCgi(std::move(uri), Cgi))
    , ProxyOverride(proxyOverride)
    , Type(NNeh::NHttp::ERequestType::Get)
    , RequestLogger(requestLogger)
    , OverrideHttpAdapterReqId(overrideHttpAdapterReqId)
{
}

TRequest& TNehRequest::AddHeader(TStringBuf key, TStringBuf value) {
    AddHeaderToString(key, value, Headers);
    return *this;
}

TVector<TString> TNehRequest::GetHeaders() const {
    return StringSplitter(Headers).SplitBySet(CRLF.data()).SkipEmpty();
}

bool TNehRequest::HasHeader(TStringBuf name) const {
    TStringStream headerStream(Headers);
    return THttpHeaders(&headerStream).HasHeader(name);
}

const TString& TNehRequest::GetBody() const {
    return Body;
}

void TNehRequest::SetPath(TStringBuf path) {
    NUri::TUriUpdate(Uri).Set(NUri::TField::FieldPath, path);
}

TString TNehRequest::GetMethod() const {
        return ToString(Type);
}

TRequest& TNehRequest::SetProxy(const TString& proxy) {
    auto newProxy = MakeIntrusive<TProxySettings>(proxy, TProxySettings::EProxyMode::Normal);
    if (ProxyOverride) {
        newProxy->ViaProxy(ProxyOverride);
    }
    ProxyOverride = newProxy;
    return *this;
}

TRequest& TNehRequest::SetMethod(TStringBuf method) {
    if (method) {
        if (NHttpMethods::POST == method) {
            Type = NNeh::NHttp::ERequestType::Post;
        }
        else if (NHttpMethods::GET == method) {
            Type = NNeh::NHttp::ERequestType::Get;
        }
        else if (NHttpMethods::PUT == method) {
            Type = NNeh::NHttp::ERequestType::Put;
        }
        else if (NHttpMethods::DELETE == method) {
            Type = NNeh::NHttp::ERequestType::Delete;
        }
        else {
            // XXX really?!
            ythrow yexception() << "neh is not supported method " << method;
        }
    }
    return *this;
}

TRequest& TNehRequest::SetBody(TStringBuf body, TStringBuf method) {
    SetMethod(method);
    Body = body;
    return *this;
}

TString TNehRequest::Url() const {
    return UrlWithCgiParams(Uri, Cgi);
}

TRequest& TNehRequest::SetContentType(TStringBuf value) {
    ContentType = value;
    return *this;
}

TNehAttemptFactory TNehRequest::MakeNehAttemptFactory() {
    static const auto defaultContentType = TString(NNeh::NHttp::DefaultContentType);
    return TNehAttemptFactory(Uri, Cgi, Headers, Body, ContentType ? ContentType : defaultContentType,
                              ProxyOverride, Type, RequestLabel, RequestLogger, OverrideHttpAdapterReqId);
}

// static
NUri::TUri TNehRequest::ConstructUriAndCgi(NUri::TUri uri, TCgiParameters& cgi) {
    cgi.Scan(uri.GetField(NUri::TField::FieldQuery));
    uri.FldClr(NUri::TField::FieldQuery);
    return uri;
}


} // namespace NHttpFetcher
