#include "iot_client.h"

#include <apphost/lib/dns/dns.h>

#include <library/cpp/neh/http_common.h>

#include <util/system/guard.h>

namespace NMatrix {

namespace {

static const TString GET_USER_INFO_RTLOG_ACTIVATION_LABEL = "iot-get-user-info";

static constexpr TStringBuf GET_USER_INFO_HOST_HEADER_NAME = "Host";
static constexpr TStringBuf GET_USER_INFO_ACCEPT_HEADER_NAME = "Accept";
static constexpr TStringBuf GET_USER_INFO_ACCEPT_HEADER_VALUE = "application/protobuf";
static constexpr TStringBuf GET_USER_INFO_PATH = "/v1.0/user/info";

static const TString IOT_CLIENT_TVM_ALIAS = "iot";

static constexpr TStringBuf BUILD_FULL_REQUEST = "build_full_request";
static constexpr TStringBuf GET_SERVICE_TICKET = "get_service_ticket";
static constexpr TStringBuf IOT_CLIENT = "iot_client";
static constexpr TStringBuf RESOLVE_ENDPOINT = "resolve_endpoint";
static constexpr TStringBuf SEND_REQUEST = "send_request";


} // namespace

TIoTClient::TIoTClient(
    const TIoTClientSettings& config,
    TTvmClient& tvmClient
)
    : Host_(config.GetHost())
    , Port_(config.GetPort())
    , Timeout_(FromString<TDuration>(config.GetTimeout()))
    , TvmClient_(tvmClient)
{}

NThreading::TFuture<TExpected<NAlice::TIoTUserInfo, TString>> TIoTClient::GetUserInfo(
    const TString& userTicket,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    TAtomicSharedPtr<TRtLogActivation> rtLogActivation =
        logContext.RtLogPtr()
        ? MakeAtomicShared<TRtLogActivation>(
            logContext.RtLogPtr(),
            GET_USER_INFO_RTLOG_ACTIVATION_LABEL,
            /* newRequest = */ false
        )
        : MakeAtomicShared<TRtLogActivation>()
    ;

    // Resolve implicitly, because Neh caches endpoint forever.
    TString endpoint;
    try {
        endpoint = NAppHost::ResolveEndpoint(Host_, Port_);
        metrics.PushRate(RESOLVE_ENDPOINT, "ok", IOT_CLIENT);
    } catch (...) {
        metrics.PushRate(RESOLVE_ENDPOINT, "error", IOT_CLIENT);

        const auto error = CurrentExceptionMessage();

        NEvClass::TMatrixIoTClientSendRequestError event;
        event.SetErrorMessage(error);
        logContext.LogEventErrorCombo<NEvClass::TMatrixIoTClientSendRequestError>(event);

        rtLogActivation->Finish(/* ok = */ false, error);

        return NThreading::MakeFuture<TExpected<NAlice::TIoTUserInfo, TString>>(error);
    }

    TString serviceTicket = "";
    if (auto serviceTicketGetResult = TvmClient_.GetServiceTicketFor(IOT_CLIENT_TVM_ALIAS)) {
        serviceTicket = serviceTicketGetResult.Success().Ticket;
        metrics.PushRate(GET_SERVICE_TICKET, "ok", IOT_CLIENT);
    } else {
        metrics.PushRate(GET_SERVICE_TICKET, "error", IOT_CLIENT);

        const auto error = serviceTicketGetResult.Error();

        NEvClass::TMatrixIoTClientSendRequestError event;
        event.SetErrorMessage(error);
        logContext.LogEventErrorCombo<NEvClass::TMatrixIoTClientSendRequestError>(event);

        rtLogActivation->Finish(/* ok = */ false, error);

        return NThreading::MakeFuture<TExpected<NAlice::TIoTUserInfo, TString>>(error);
    }

    auto message = NNeh::TMessage(
        TString::Join("http://", endpoint, GET_USER_INFO_PATH),
        /* data = */ {}
    );

    TString headers = TString::Join(
        "\r\n", GET_USER_INFO_HOST_HEADER_NAME, ": ", Host_,
        "\r\n", GET_USER_INFO_ACCEPT_HEADER_NAME, ": ", GET_USER_INFO_ACCEPT_HEADER_VALUE,
        "\r\n", TTvmClient::USER_TICKET_HEADER_NAME, ": ", userTicket,
        "\r\n", TTvmClient::SERVICE_TICKET_HEADER_NAME, ": ", serviceTicket,
        (rtLogActivation->Token().empty())
            ? ""
            : TString::Join("\r\nX-RTLog-Token: ", rtLogActivation->Token())
    );

    logContext.LogEventInfoCombo<NEvClass::TMatrixIoTClientRequest>(Host_, Port_, endpoint);

    if (!NNeh::NHttp::MakeFullRequest(message, headers, /* content = */ "", /* contentType = */ "", NNeh::NHttp::ERequestType::Get)) {
        metrics.PushRate(BUILD_FULL_REQUEST, "error", IOT_CLIENT);

        static const TString error = "Unable to build full request";

        NEvClass::TMatrixIoTClientSendRequestError event;
        event.SetErrorMessage(error);
        logContext.LogEventErrorCombo<NEvClass::TMatrixIoTClientSendRequestError>(event);

        rtLogActivation->Finish(/* ok = */ false, error);

        return NThreading::MakeFuture<TExpected<NAlice::TIoTUserInfo, TString>>(error);
    } else {
        metrics.PushRate(BUILD_FULL_REQUEST, "ok", IOT_CLIENT);
    }

    const auto reqStarted = TInstant::Now();
    NAppHost::NTransport::TRequestHandlePtr requestHandle = Ncs_.SendRequest(message);

    // Enable deadlineTimer before GetFuture().Apply(...) to prevent race
    TAtomicSharedPtr<NAsio::TDeadlineTimer> deadlineTimer = MakeAtomicShared<NAsio::TDeadlineTimer>(DeadlineTimersExecutor_.GetIOService());
    deadlineTimer->AsyncWaitExpireAt(
        Timeout_,
        [requestHandle](const NAsio::TErrorCode& errorCode, NAsio::IHandlingContext&) {
            if (!errorCode) {
                requestHandle->Cancel();
            }
        }
    );

    return requestHandle->GetFuture().Apply(
        [this, &metrics, logContext, reqStarted, rtLogActivation = std::move(rtLogActivation), deadlineTimer = std::move(deadlineTimer)](
            const NAppHost::NTransport::TResponseHandle& respFut
        ) mutable -> TExpected<NAlice::TIoTUserInfo, TString> {
            deadlineTimer->Cancel();

            NAppHost::NTransport::TResponsePtr resp = nullptr;
            try {
                resp = respFut.GetValue(Timeout_);
                metrics.PushDurationHist(
                    TInstant::Now() - reqStarted,
                    "request_time",
                    /* code = */ "",
                    IOT_CLIENT
                );
            } catch (...) {
                const auto error = CurrentExceptionMessage();

                NEvClass::TMatrixIoTClientSendRequestError event;
                event.SetErrorMessage(error);
                logContext.LogEventErrorCombo<NEvClass::TMatrixIoTClientSendRequestError>(event);

                rtLogActivation->Finish(/* ok = */ false, CurrentExceptionMessage());

                metrics.SetError("send_subway_request");

                return error;
            }

            if (!resp->GetErrorMessage().empty() || resp->IsTimedOut()) {
                NEvClass::TMatrixIoTClientSendRequestError event;

                if (resp->IsTimedOut() || resp->GetErrorType() == NNeh::TError::TType::Cancelled /* cancel by deadline timer */) {
                    metrics.PushRate(SEND_REQUEST, "timeout", IOT_CLIENT);
                    event.SetIsTimeOut(true);
                } else if (resp->GetErrorType() == NNeh::TError::TType::ProtocolSpecific) {
                    metrics.PushRate(SEND_REQUEST, "protocol_specific_error", IOT_CLIENT);
                } else {
                    // We have only cancelled, protocol specific and unknown error types: https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/neh/neh.h?rev=r8540110#L34-38
                    metrics.PushRate(SEND_REQUEST, "unknown_error", IOT_CLIENT);
                }

                event.SetErrorMessage(resp->GetErrorMessage());
                logContext.LogEventErrorCombo<NEvClass::TMatrixIoTClientSendRequestError>(event);

                rtLogActivation->Finish(/* ok = */ false, resp->GetErrorMessage());

                return resp->GetErrorMessage();
            } else {
                int code = resp->GetErrorCode() ? resp->GetErrorCode() : 200;
                metrics.RateHttpCode(1, code, IOT_CLIENT);

                NAlice::TIoTUserInfo iotResponse;
                if (iotResponse.ParseFromArray(resp->GetData().data(), resp->GetData().size())) {

                    NEvClass::TMatrixIoTClientSendRequestSuccess event;
                    event.SetHttpResponseCode(code);
                    event.MutableIoTProtoResponse()->CopyFrom(iotResponse);
                    logContext.LogEventInfoCombo<NEvClass::TMatrixIoTClientSendRequestSuccess>(event);

                    rtLogActivation->Finish(/* ok = */ true);

                    metrics.PushRate("response_proto_parse", "ok", IOT_CLIENT);

                    return iotResponse;
                } else {
                    static const TString error = "Unable to parse iot porto response";

                    NEvClass::TMatrixIoTClientSendRequestError event;
                    event.SetHttpResponseCode(code);
                    event.SetIoTUnparsedRawResponse(TString(resp->GetData()));
                    event.SetErrorMessage(error);
                    logContext.LogEventErrorCombo<NEvClass::TMatrixIoTClientSendRequestError>(event);

                    rtLogActivation->Finish(/* ok = */ false, error);

                    metrics.PushRate("response_proto_parse", "error", IOT_CLIENT);

                    return error;
                }
            }
        }
    );
}

} // namespace NMatrix
