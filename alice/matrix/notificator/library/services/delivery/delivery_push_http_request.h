#pragma once

#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>
#include <alice/matrix/notificator/library/user_white_list/user_white_list.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <alice/protos/api/matrix/delivery.pb.h>

#include <alice/matrix/library/request/http_request.h>


namespace NMatrix::NNotificator {

class TDeliveryPushHttpRequest : public TProtoHttpRequestWithProtoResponse<
    NMatrix::NApi::TDelivery,
    NApi::TDeliveryResponse,
    NEvClass::TMatrixNotificatorDeliveryPushHttpRequestData,
    NEvClass::TMatrixNotificatorDeliveryPushHttpResponseData
> {
public:
    TDeliveryPushHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TPushesAndNotificationsClient& pushesAndNotificationsClient,
        const TUserWhiteList& userWhiteList
    );
    ~TDeliveryPushHttpRequest();

    NThreading::TFuture<void> ServeAsync() override;

private:
    void SetAddPushToDatabaseStatus(
        NApi::TDeliveryResponse::TAddPushToDatabaseStatus::EStatus status,
        TMaybe<TString> errorMessage = Nothing()
    );
    void SetSubwayRequestStatus(
        NApi::TDeliveryResponse::TSubwayRequestStatus::EStatus status,
        TMaybe<TString> errorMessage = Nothing()
    );

    NThreading::TFuture<void> OnAddDirectiveResult(
        const NThreading::TFuture<TExpected<void, TString>>& addDirectiveResultFut
    );

    NThreading::TFuture<void> OnListConnectionsResult(
        const NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>>& listConnectionsResultFut
    );

public:
    static inline constexpr TStringBuf NAME = "delivery_push";
    static inline constexpr TStringBuf PATH = "/delivery/push";

private:
    TPushesAndNotificationsClient& PushesAndNotificationsClient_;
    const TUserWhiteList& UserWhiteList_;

    TDirectivesStorage::TDirective Directive_;

    // Analytics part
    NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::EResult TechnicalPushValidationResult_;
};

} // namespace NMatrix::NNotificator
