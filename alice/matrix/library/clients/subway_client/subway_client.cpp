#include "subway_client.h"

#include <apphost/lib/dns/dns.h>

#include <library/cpp/neh/http_common.h>

#include <util/system/guard.h>

namespace NMatrix {

namespace {

static constexpr TStringBuf REQUEST_CONTENT_TYPE = "application/octet-stream";

static constexpr TStringBuf SUBWAY_CLIENT = "subway_client";
static constexpr TStringBuf SEND_REQUEST = "send_request";
static constexpr TStringBuf BUILD_FULL_REQUEST = "build_full_request";
static constexpr TStringBuf RESOLVE_ENDPOINT = "resolve_endpoint";

} // namespace

TSubwayClient::TSubwayClient(
    const TSubwayClientSettings& config
)
    : Timeout_(FromString<TDuration>(config.GetTimeout()))
    , HostOrIpOverride_(config.HasHardcodedForTestsHostOrIp() ? TMaybe<TString>(config.GetHardcodedForTestsHostOrIp()) : Nothing())
    , PortOverride_(config.HasHardcodedForTestsPort() ? TMaybe<ui32>(config.GetHardcodedForTestsPort()) : Nothing())
{}

NThreading::TFuture<TExpected<NUniproxy::TSubwayResponse, TString>> TSubwayClient::SendSubwayMessage(
    const NUniproxy::TSubwayMessage& subwayMessage,
    const TString& puid,
    TString hostOrIp,
    ui32 port,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    hostOrIp = HostOrIpOverride_.GetOrElse(hostOrIp);
    port = PortOverride_.GetOrElse(port);

    TAtomicSharedPtr<TRtLogActivation> rtLogActivation =
        logContext.RtLogPtr()
        ? MakeAtomicShared<TRtLogActivation>(
            logContext.RtLogPtr(),
            TString::Join("subway-", hostOrIp, ':', ToString(port)),
            /* newRequest = */ false
        )
        : MakeAtomicShared<TRtLogActivation>()
    ;

    // Resolve implicitly, because Neh caches endpoint forever.
    TString endpoint;
    try {
        endpoint = NAppHost::ResolveEndpoint(hostOrIp, port);
        metrics.PushRate(RESOLVE_ENDPOINT, "ok", SUBWAY_CLIENT);
    } catch (...) {
        metrics.PushRate(RESOLVE_ENDPOINT, "error", SUBWAY_CLIENT);

        const auto error = CurrentExceptionMessage();

        NEvClass::TMatrixSubwayClientSendRequestError event;
        event.SetErrorMessage(error);
        logContext.LogEventErrorCombo<NEvClass::TMatrixSubwayClientSendRequestError>(event);

        rtLogActivation->Finish(/* ok = */ false, error);

        return NThreading::MakeFuture<TExpected<NUniproxy::TSubwayResponse, TString>>(error);
    }

    for (const auto& pushId : subwayMessage.GetQuasarMsg().GetPushIds()) {
        for (const auto& destination : subwayMessage.GetDestinations()) {
            // Important log for analytics
            logContext.LogEventInfoCombo<NEvClass::TMatrixSubwayClientSendDirectiveToSubway>(
                pushId,
                endpoint,
                puid,
                destination.GetDeviceId()
            );
        }
    }

    auto message = NNeh::TMessage(
        TString::Join("http://", endpoint, "/push"),
        /* data = */ {}
    );

    TString headers = (rtLogActivation->Token().empty())
        ? ""
        : TString::Join("\r\nX-RTLog-Token: ", rtLogActivation->Token())
    ;

    logContext.LogEventInfoCombo<NEvClass::TMatrixSubwayClientRequest>(hostOrIp, port, subwayMessage, endpoint);

    if (!NNeh::NHttp::MakeFullRequest(message, headers, subwayMessage.SerializeAsString(), REQUEST_CONTENT_TYPE, NNeh::NHttp::ERequestType::Post)) {
        metrics.PushRate(BUILD_FULL_REQUEST, "error", SUBWAY_CLIENT);

        static const TString error = "Unable to build full request";

        NEvClass::TMatrixSubwayClientSendRequestError event;
        event.SetErrorMessage(error);
        logContext.LogEventErrorCombo<NEvClass::TMatrixSubwayClientSendRequestError>(event);

        rtLogActivation->Finish(/* ok = */ false, error);

        return NThreading::MakeFuture<TExpected<NUniproxy::TSubwayResponse, TString>>(error);
    } else {
        metrics.PushRate(BUILD_FULL_REQUEST, "ok", SUBWAY_CLIENT);
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
        ) mutable -> TExpected<NUniproxy::TSubwayResponse, TString> {
            deadlineTimer->Cancel();

            NAppHost::NTransport::TResponsePtr resp = nullptr;
            try {
                resp = respFut.GetValue(Timeout_);
                metrics.PushDurationHist(
                    TInstant::Now() - reqStarted,
                    "request_time",
                    /* code = */ "",
                    SUBWAY_CLIENT
                );
            } catch (...) {
                const auto error = CurrentExceptionMessage();

                NEvClass::TMatrixSubwayClientSendRequestError event;
                event.SetErrorMessage(error);
                logContext.LogEventErrorCombo<NEvClass::TMatrixSubwayClientSendRequestError>(event);

                rtLogActivation->Finish(/* ok = */ false, CurrentExceptionMessage());

                metrics.SetError("send_subway_request");

                return error;
            }

            if (!resp->GetErrorMessage().empty() || resp->IsTimedOut()) {
                NEvClass::TMatrixSubwayClientSendRequestError event;

                if (resp->IsTimedOut() || resp->GetErrorType() == NNeh::TError::TType::Cancelled /* cancel by deadline timer */) {
                    metrics.PushRate(SEND_REQUEST, "timeout", SUBWAY_CLIENT);
                    event.SetIsTimeOut(true);
                } else if (resp->GetErrorType() == NNeh::TError::TType::ProtocolSpecific) {
                    metrics.PushRate(SEND_REQUEST, "protocol_specific_error", SUBWAY_CLIENT);
                } else {
                    // We have only cancelled, protocol specific and unknown error types: https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/neh/neh.h?rev=r8540110#L34-38
                    metrics.PushRate(SEND_REQUEST, "unknown_error", SUBWAY_CLIENT);
                }

                event.SetErrorMessage(resp->GetErrorMessage());
                logContext.LogEventErrorCombo<NEvClass::TMatrixSubwayClientSendRequestError>(event);

                rtLogActivation->Finish(/* ok = */ false, resp->GetErrorMessage());

                return resp->GetErrorMessage();
            } else {
                int code = resp->GetErrorCode() ? resp->GetErrorCode() : 200;
                metrics.RateHttpCode(1, code, SUBWAY_CLIENT);

                NUniproxy::TSubwayResponse subwayResponse;
                if (subwayResponse.ParseFromArray(resp->GetData().data(), resp->GetData().size())) {

                    NEvClass::TMatrixSubwayClientSendRequestSuccess event;
                    event.SetHttpResponseCode(code);
                    event.MutableSubwayProtoResponse()->CopyFrom(subwayResponse);
                    logContext.LogEventInfoCombo<NEvClass::TMatrixSubwayClientSendRequestSuccess>(event);

                    rtLogActivation->Finish(/* ok = */ true);

                    metrics.PushRate("response_proto_parse", "ok", SUBWAY_CLIENT);
                    metrics.PushRate(subwayResponse.GetMissingDevices().size(), "response_missing_devices_count", "" /* code */, SUBWAY_CLIENT);
                    metrics.PushRate("response_status", ToString(subwayResponse.GetStatus()), SUBWAY_CLIENT);

                    return subwayResponse;
                } else {
                    static const TString error = "Unable to parse subway porto response";

                    NEvClass::TMatrixSubwayClientSendRequestError event;
                    event.SetHttpResponseCode(code);
                    event.SetSubwayUnparsedRawResponse(TString(resp->GetData()));
                    event.SetErrorMessage(error);
                    logContext.LogEventErrorCombo<NEvClass::TMatrixSubwayClientSendRequestError>(event);

                    rtLogActivation->Finish(/* ok = */ false, error);

                    metrics.PushRate("response_proto_parse", "error", SUBWAY_CLIENT);

                    return error;
                }
            }
        }
    );
}

} // namespace NMatrix
