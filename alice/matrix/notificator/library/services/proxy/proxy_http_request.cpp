#include "proxy_http_request.h"


namespace NMatrix::NNotificator {

namespace {

static constexpr TStringBuf PY_NOTIFICATOR = "py_notificator";
static constexpr TStringBuf EMPTY_GET_ADDRESS  = "empty_get_address";
static constexpr TStringBuf BUILD_FULL_REQUEST  = "build_full_request";

} // namespace

TProxyHttpRequest::TProxyHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    const TString& destinationServiceName,
    const THolder<TSDClientBase>& sdClient,
    const TDuration timeout,
    NAppHost::NTransport::TNehCommunicationSystem& ncs,
    NAsio::TIOService& deadlineTimersIOService
)
    : THttpRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ false,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            return NNeh::NHttp::ERequestType::Get == method ||
                   NNeh::NHttp::ERequestType::Post == method ||
                   NNeh::NHttp::ERequestType::Delete == method;
        }
    )
    , Timeout_(timeout)
    , ProxyRequestRTLogActivation_(
        LogContext_.RtLogPtr()
        ? TRtLogActivation(
            LogContext_.RtLogPtr(),
            destinationServiceName,
            /* newRequest = */ false
        )
        : TRtLogActivation()
    )
    , CodeFromBackend_(200)
    , Ncs_(ncs)
    , DeadlineTimer_(deadlineTimersIOService)
{
    if (IsFinished()) {
        return;
    }

    auto address = sdClient->GetAddress();
    if (!address.Defined()) {
        Metrics_.PushRate(EMPTY_GET_ADDRESS, "error", Path_);

        const TString error = TString::Join("Unable to get '", destinationServiceName, "' address");

        LogContext_.LogEventErrorCombo<NEvClass::TMatrixNotificatorProxyServiceSendRequestError>(error, false);
        ProxyRequestRTLogActivation_.Finish(/* ok = */ false, error);
        SetError(error, 500);

        IsFinished_ = true;
        return;
    }

    const auto [backendAddress, isLocalDc] = *sdClient->GetAddress();
    if (Y_UNLIKELY(!isLocalDc)) {
        Metrics_.PushRate("cross_dc_count", "warning", Path_);
    }
    LogContext_.LogEventInfoCombo<NEvClass::TMatrixNotificatorProxyServiceBackend>(backendAddress, isLocalDc);

    TStringStream headers;
    {
        if (!ProxyRequestRTLogActivation_.Token().empty()) {
            THttpHeaders newHeaders = HttpRequest_->Headers();
            newHeaders.AddOrReplaceHeader("X-RTLog-Token", ProxyRequestRTLogActivation_.Token());
            newHeaders.OutTo(&headers);
        } else {
            // Do not copy headers without rtlog activation
            HttpRequest_->Headers().OutTo(&headers);
        }
    }
    const THttpInputHeader *contentTypeHeader = HttpRequest_->Headers().FindHeader("Content-Type");
    // https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/notificator/common_handler.py?rev=r8174121#L19
    const TStringBuf contentTypeValue = contentTypeHeader ? TStringBuf(contentTypeHeader->Value()) : "application/protobuf";

    const TString url = TString::Join(
        backendAddress, '/', Path_,
        HttpRequest_->Cgi().empty()
            ? ""
            : TString::Join("?", HttpRequest_->Cgi())
    );

    // We dont resolve endpoint here, because it is already resolved ipv6 from SDClient.
    Message_ = NNeh::TMessage(url, {} /* data */);
    if (!NNeh::NHttp::MakeFullRequest(Message_, headers.Str(), HttpRequest_->Body(), contentTypeValue, Method_)) {
        Metrics_.PushRate(BUILD_FULL_REQUEST, "error", Path_);

        static const TString error = "Unable to build full request";

        LogContext_.LogEventErrorCombo<NEvClass::TMatrixNotificatorProxyServiceSendRequestError>(error, false);
        ProxyRequestRTLogActivation_.Finish(/* ok = */ false, error);
        SetError(error, 500);

        IsFinished_ = true;
        return;
    } else {
        Metrics_.PushRate(BUILD_FULL_REQUEST, "ok", Path_);
    }
}

NThreading::TFuture<void> TProxyHttpRequest::ServeAsync() {
    const auto reqStarted = TInstant::Now();
    NAppHost::NTransport::TRequestHandlePtr requestHandle = Ncs_.SendRequest(Message_);

    // Enable DeadlineTimer_ before GetFuture().Apply(...) to prevent race
    DeadlineTimer_.AsyncWaitExpireAt(
        Timeout_,
        [requestHandle](const NAsio::TErrorCode& errorCode, NAsio::IHandlingContext&) {
            if (!errorCode) {
                requestHandle->Cancel();
            }
        }
    );

    return requestHandle->GetFuture().Apply(
        [this, reqStarted] (const NAppHost::NTransport::TResponseHandle& respFut) {
            DeadlineTimer_.Cancel();

            NAppHost::NTransport::TResponsePtr resp = nullptr;
            try {
                resp = respFut.GetValueSync();
                Metrics_.PushDurationHist(
                    TInstant::Now() - reqStarted,
                    "request_time",
                    "", /* code */
                    TString::Join(PY_NOTIFICATOR, "_", Path_)
                );
            } catch (...) {
                const auto error = CurrentExceptionMessage();

                LogContext_.LogEventErrorCombo<NEvClass::TMatrixNotificatorProxyServiceSendRequestError>(error, false);
                ProxyRequestRTLogActivation_.Finish(/* ok = */ false, error);

                Metrics_.SetError("send_request");
                SetError("Failed to send request to backend", 500);

                IsFinished_ = true;
                return;
            }

            if (!resp->GetErrorMessage().empty() || resp->IsTimedOut()) {
                NEvClass::TMatrixNotificatorProxyServiceSendRequestError event;

                if (resp->IsTimedOut() || resp->GetErrorType() == NNeh::TError::TType::Cancelled /* cancel by deadline timer */) {
                    event.SetIsTimeOut(true);
                    Metrics_.PushRate("send_request", "timeout", Path_);
                } else if (resp->GetErrorType() == NNeh::TError::TType::ProtocolSpecific) {
                    Metrics_.PushRate("send_request", "protocol_specific_error", Path_);
                } else {
                    // We have only cancelled, protocol specific and unknown error types: https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/neh/neh.h?rev=r8540110#L34-38
                    Metrics_.PushRate("send_request", "unknown_error", Path_);
                }

                event.SetErrorMessage(resp->GetErrorMessage());
                LogContext_.LogEventErrorCombo<NEvClass::TMatrixNotificatorProxyServiceSendRequestError>(event);

                ProxyRequestRTLogActivation_.Finish(/* ok = */ false, resp->GetErrorMessage());

                Metrics_.SetError("send_request");
                SetError(resp->GetErrorMessage(), resp->GetErrorCode() ? resp->GetErrorCode() : 500);

                IsFinished_ = true;
                return;
            }

            CodeFromBackend_ = resp->GetErrorCode() ? resp->GetErrorCode() : 200;
            DataFromBackend_ = resp->GetData();

            for (const auto& header : resp->GetHeaders()) {
                /* 'Transfer-Encoding' header is not needed
                 * because 'resp->GetData()' is already decoded.
                 */
                if ("Transfer-Encoding" == header.Name()) {
                    continue;
                }

                /* 'Content-Length' header's value is not
                 * always equals to real content length.
                 */
                if ("Content-Length" == header.Name()) {
                    continue;
                }

                HeadersFromBackend_.AddHeader(header);
            }

            ProxyRequestRTLogActivation_.Finish(/* ok = */ true);
            IsFinished_ = true;
        }
    );
}

TProxyHttpRequest::TReply TProxyHttpRequest::GetReply() const {
    return {DataFromBackend_, HeadersFromBackend_, CodeFromBackend_};
}

} // namespace NMatrix::NNotificator
