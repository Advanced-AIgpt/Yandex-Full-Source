#pragma once

#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>

#include <alice/matrix/library/clients/iot_client/iot_client.h>
#include <alice/matrix/library/request/http_request.h>


namespace NMatrix::NNotificator {

class TSubscriptionsHttpRequest : public THttpRequest<
    NEvClass::TMatrixNotificatorSubscriptionsHttpRequestData,
    NEvClass::TMatrixNotificatorSubscriptionsHttpResponseData
> {
public:
    TSubscriptionsHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TIoTClient& iotClient,
        TPushesAndNotificationsClient& pushesAndNotificationsClient
    );

    NThreading::TFuture<void> ServeAsync() override;

    bool NeedTvmServiceTicket() const;

private:
    TReply GetReply() const override;

    TString GetReplyData(
        const TMap<ui64, TSubscriptionsStorage::TUserSubscription>& userSubscriptions,
        const NAlice::TIoTUserInfo& iotUserInfo
    ) const;

public:
    static inline constexpr TStringBuf NAME = "subscriptions";
    static inline constexpr TStringBuf PATH = "/subscriptions";

private:
    TIoTClient& IoTClient_;
    TPushesAndNotificationsClient& PushesAndNotificationsClient_;

    TString Puid_;
    TMaybe<TString> UserTicket_;

    TSubscriptionsHttpRequest::TReply HttpReply_;
};

} // namespace NMatrix::NNotificator
