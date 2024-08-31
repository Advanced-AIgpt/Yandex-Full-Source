#pragma once

#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>
#include <alice/matrix/notificator/library/user_white_list/user_white_list.h>

#include <alice/matrix/library/request/http_request.h>


namespace NMatrix::NNotificator {

class TDeliveryDemoHttpRequest : public THttpRequest<
    NEvClass::TMatrixNotificatorDeliveryDemoHttpRequestData,
    NEvClass::TMatrixNotificatorDeliveryDemoHttpResponseData
> {
public:
    TDeliveryDemoHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TPushesAndNotificationsClient& pushesAndNotificationsClient,
        const TUserWhiteList& userWhiteList
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    TReply GetReply() const override;

    TString GetReplyData(bool noLocations) const;

public:
    static inline constexpr TStringBuf NAME = "delivery_demo";
    static inline constexpr TStringBuf PATH = "/delivery/demo";

private:
    TPushesAndNotificationsClient& PushesAndNotificationsClient_;
    const TUserWhiteList& UserWhiteList_;

    TString Puid_;
    ui64 SubscriptionId_;
    TNotificationsStorage::TNotification Notification_;

    TDeliveryDemoHttpRequest::TReply HttpReply_;
};

} // namespace NMatrix::NNotificator
