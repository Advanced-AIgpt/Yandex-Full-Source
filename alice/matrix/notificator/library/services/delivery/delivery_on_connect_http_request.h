#pragma once

#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <alice/uniproxy/library/protos/notificator.pb.h>

#include <alice/matrix/library/request/http_request.h>


namespace NMatrix::NNotificator {

class TDeliveryOnConnectHttpRequest : public TProtoHttpRequest<
    ::NNotificator::TDeliveryOnConnect,
    NEvClass::TMatrixNotificatorDeliveryOnConnectHttpRequestData,
    NEvClass::TMatrixNotificatorDeliveryOnConnectHttpResponseData
> {
public:
    TDeliveryOnConnectHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TPushesAndNotificationsClient& pushesAndNotificationsClient
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    TReply GetReply() const override;

    TString GetReplyData() const;
    TConnectionsStorage::TListConnectionsResult GetListConnectionsResult() const;

    NThreading::TFuture<void> OnDirectivesResult(
        const NThreading::TFuture<TExpected<TVector<TDirectivesStorage::TDirective>, TString>>& directivesFut
    );

    NThreading::TFuture<void> OnGetUserSubscriptionsInfo(
        const NThreading::TFuture<TExpected<TPushesAndNotificationsClient::TUserSubscriptionsInfo, TString>>& userSubscriptionsInfoResultFut,
        TVector<TDirectivesStorage::TDirective> directives
    );

    NThreading::TFuture<void> OnGetUserNotificationsInfo(
        const NThreading::TFuture<TExpected<TMaybe<TPushesAndNotificationsClient::TUserNotificationsInfo>, TString>>& userNotificationsInfoFut,
        TVector<TDirectivesStorage::TDirective> directives
    );

public:
    static inline constexpr TStringBuf NAME = "delivery_on_connect";
    static inline constexpr TStringBuf PATH = "/delivery/on_connect";

private:
    TPushesAndNotificationsClient& PushesAndNotificationsClient_;
};

} // namespace NMatrix::NNotificator
