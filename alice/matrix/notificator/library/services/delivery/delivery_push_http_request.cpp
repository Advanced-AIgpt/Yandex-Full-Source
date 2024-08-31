#include "delivery_push_http_request.h"

#include <alice/megamind/api/utils/directives.h>

#include <util/generic/guid.h>

namespace NMatrix::NNotificator {

namespace {

using std::placeholders::_1;

} // namespace

TDeliveryPushHttpRequest::TDeliveryPushHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TPushesAndNotificationsClient& pushesAndNotificationsClient,
    const TUserWhiteList& userWhiteList
)
    : THttpRequestWithProtoResponse(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ true,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            return NNeh::NHttp::ERequestType::Post == method;
        }
    )
    , PushesAndNotificationsClient_(pushesAndNotificationsClient)
    , UserWhiteList_(userWhiteList)

    , Directive_()

    , TechnicalPushValidationResult_(NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::UNKNOWN)
{
    if (IsFinished()) {
        return;
    }

    Directive_.PushId = (Request_->GetPushId().empty() ? TGUID::Create().AsUuidString() : TString(Request_->GetPushId()));

    LogContext_.LogEventInfoCombo<NEvClass::TMatrixNotificatorAnalyticsNewTechnicalPush>(
        Directive_.PushId
    );

    if (Request_->GetPuid().empty()) {
        SetResponseHttpCode(400);
        SetAddPushToDatabaseStatus(
            NApi::TDeliveryResponse::TAddPushToDatabaseStatus::ERROR,
            "Puid is empty"
        );
        TechnicalPushValidationResult_ = NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::PUID_IS_EMPTY;
        IsFinished_ = true;
        return;
    }

    if (Request_->GetDeviceId().empty()) {
        SetResponseHttpCode(400);
        SetAddPushToDatabaseStatus(
            NApi::TDeliveryResponse::TAddPushToDatabaseStatus::ERROR,
            "Device id is empty"
        );
        TechnicalPushValidationResult_ = NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::DEVICE_ID_IS_EMPTY;
        IsFinished_ = true;
        return;
    }

    // Important log for analytics
    LogContext_.LogEventInfoCombo<NEvClass::TMatrixNotificatorAddDirectiveToDatabase>(
        Directive_.PushId,
        Request_->GetPuid(),
        Request_->GetDeviceId()
    );

    switch (Request_->GetTRequestDirectiveCase()) {
        case NMatrix::NApi::TDelivery::kSemanticFrameRequestData: {
            Directive_.SpeechKitDirective = NAlice::NMegamindApi::MakeDirectiveWithTypedSemanticFrame(Request_->GetSemanticFrameRequestData());
            break;
        }
        case NMatrix::NApi::TDelivery::kSpeechKitDirective: {
            if (!Directive_.SpeechKitDirective.ParseFromString(Request_->GetSpeechKitDirective())) {
                SetResponseHttpCode(400);
                SetAddPushToDatabaseStatus(
                    NApi::TDeliveryResponse::TAddPushToDatabaseStatus::ERROR,
                    "Failed to parse speech kit directive"
                );
                TechnicalPushValidationResult_ = NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::BAD_TECHNICAL_PUSH_CONTENT;
                Metrics_.PushRate("speech_kit_directive_parse_error", "error");
                IsFinished_ = true;
                return;
            }

            break;
        }
        case NMatrix::NApi::TDelivery::TREQUESTDIRECTIVE_NOT_SET: {
            SetResponseHttpCode(400);
            SetAddPushToDatabaseStatus(
                NApi::TDeliveryResponse::TAddPushToDatabaseStatus::ERROR,
                "TRequestDirective not supported or not set"
            );
            TechnicalPushValidationResult_ = NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::NO_TECHNICAL_PUSH_IN_REQUEST;
            IsFinished_ = true;
            return;
        }
    }

    if (!UserWhiteList_.IsPuidAllowedToProcess(Request_->GetPuid())) {
        SetResponseHttpCode(403);
        SetAddPushToDatabaseStatus(
            NApi::TDeliveryResponse::TAddPushToDatabaseStatus::ERROR,
            "Puid from request is not in white list"
        );
        TechnicalPushValidationResult_ = NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::PUID_IS_NOT_ALLOWED_TO_PROCESS;
        Metrics_.PushRate("puid_not_allowed_to_process", "error");
        IsFinished_ = true;
        return;
    }

    TechnicalPushValidationResult_ = NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::SUCCESS;
}

TDeliveryPushHttpRequest::~TDeliveryPushHttpRequest() {
    try {
        Metrics_.PushRate(
            "add_push_to_database_status",
            to_lower(NApi::TDeliveryResponse::TAddPushToDatabaseStatus::EStatus_Name(Response_.GetAddPushToDatabaseStatus().GetStatus()))
        );
        Metrics_.PushRate(
            "subway_request_status",
            to_lower(NApi::TDeliveryResponse::TSubwayRequestStatus::EStatus_Name(Response_.GetSubwayRequestStatus().GetStatus()))
        );

        LogContext_.LogEventInfoCombo<NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult>(
            Directive_.PushId,
            TechnicalPushValidationResult_
        );
    } catch (...) {
    }
}

NThreading::TFuture<void> TDeliveryPushHttpRequest::ServeAsync() {
    return PushesAndNotificationsClient_.AddDirective(
        TDirectivesStorage::TUserDevice({
            .Puid = Request_->GetPuid(),
            .DeviceId = Request_->GetDeviceId(),
        }),
        Directive_,
        TDuration::Seconds(Request_->GetTtl()),
        LogContext_,
        Metrics_
    ).Apply(
        std::bind(&TDeliveryPushHttpRequest::OnAddDirectiveResult, this, _1)
    );
}

NThreading::TFuture<void> TDeliveryPushHttpRequest::OnAddDirectiveResult(
    const NThreading::TFuture<TExpected<void, TString>>& addDirectiveResultFut
) {
    auto addDirectiveResult = addDirectiveResultFut.GetValueSync();
    if (!addDirectiveResult) {
        SetResponseHttpCode(500);
        SetAddPushToDatabaseStatus(
            NApi::TDeliveryResponse::TAddPushToDatabaseStatus::ERROR,
            addDirectiveResult.Error()
        );
        return NThreading::MakeFuture();
    }

    SetAddPushToDatabaseStatus(
        NApi::TDeliveryResponse::TAddPushToDatabaseStatus::OK
    );
    // Fill legacy api field
    Response_.SetId(Directive_.PushId);
    Response_.SetPushId(Directive_.PushId);

    return PushesAndNotificationsClient_.ListConnections(
        Request_->GetPuid(),
        Request_->GetDeviceId(),
        LogContext_,
        Metrics_
    ).Apply(
        std::bind(&TDeliveryPushHttpRequest::OnListConnectionsResult, this, _1)
    );
}

NThreading::TFuture<void> TDeliveryPushHttpRequest::OnListConnectionsResult(
    const NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>>& listConnectionsResultFut
) {
    auto listConnectionsResult = listConnectionsResultFut.GetValueSync();
    if (!listConnectionsResult) {
        SetResponseHttpCode(500);
        SetSubwayRequestStatus(
            NApi::TDeliveryResponse::TSubwayRequestStatus::LOCATION_DISCOVERY_ERROR,
            listConnectionsResult.Error()
        );
        return NThreading::MakeFuture();
    }
    auto listConnections = listConnectionsResult.Success();

    if (listConnections.Records.empty()) {
        SetSubwayRequestStatus(
            NApi::TDeliveryResponse::TSubwayRequestStatus::LOCATION_NOT_FOUND
        );
        return NThreading::MakeFuture();
    }

    return PushesAndNotificationsClient_.SendSubwayMessageToAllDevices(
        Request_->GetPuid(),
        NAlice::NScenarios::TNotifyDirective::NoSound,
        Nothing(),
        TVector<TDirectivesStorage::TDirective>({Directive_}),
        listConnections,
        LogContext_,
        Metrics_
    ).Apply(
        [this](const NThreading::TFuture<TExpected<TVector<NUniproxy::TSubwayResponse>, TString>>& fut) mutable {
            const auto& futRes = fut.GetValueSync();
            if (!futRes) {
                SetResponseHttpCode(500);
                SetSubwayRequestStatus(
                    NApi::TDeliveryResponse::TSubwayRequestStatus::REQUEST_ERROR,
                    futRes.Error()
                );
                return;
            }

            NApi::TDeliveryResponse::TSubwayRequestStatus::EStatus subwayStatus = NApi::TDeliveryResponse::TSubwayRequestStatus::OUTDATED_LOCATION;
            for (const auto& subwayResponse : futRes.Success()) {
                bool deviceIsMissed = false;
                for (const auto& deviceId : subwayResponse.GetMissingDevices()) {
                    if (deviceId == Request_->GetDeviceId()) {
                        deviceIsMissed = true;
                        break;
                    }
                }
                if (!deviceIsMissed) {
                    subwayStatus = NApi::TDeliveryResponse::TSubwayRequestStatus::OK;
                    break;
                }
            }

            SetSubwayRequestStatus(subwayStatus);
        }
    );
}

void TDeliveryPushHttpRequest::SetAddPushToDatabaseStatus(
    NApi::TDeliveryResponse::TAddPushToDatabaseStatus::EStatus status,
    TMaybe<TString> errorMessage
) {
    Response_.MutableAddPushToDatabaseStatus()->SetStatus(status);
    if (errorMessage.Defined()) {
        Response_.MutableAddPushToDatabaseStatus()->SetErrorMessage(*errorMessage);
    }
}

void TDeliveryPushHttpRequest::SetSubwayRequestStatus(
    NApi::TDeliveryResponse::TSubwayRequestStatus::EStatus status,
    TMaybe<TString> errorMessage
) {
    Response_.MutableSubwayRequestStatus()->SetStatus(status);
    if (errorMessage.Defined()) {
        Response_.MutableSubwayRequestStatus()->SetErrorMessage(*errorMessage);
    }

    // Fill legacy api field
    if (status == NApi::TDeliveryResponse::TSubwayRequestStatus::OK) {
        Response_.SetCode(NMatrix::NApi::TDeliveryResponse::OK);
    } else if (
        status == NApi::TDeliveryResponse::TSubwayRequestStatus::LOCATION_NOT_FOUND ||
        status == NApi::TDeliveryResponse::TSubwayRequestStatus::OUTDATED_LOCATION
    ) {
        Response_.SetCode(NMatrix::NApi::TDeliveryResponse::NoLocations);
    } else {
        Response_.SetCode(NMatrix::NApi::TDeliveryResponse::Unknown);
    }
}

} // namespace NMatrix::NNotificator
