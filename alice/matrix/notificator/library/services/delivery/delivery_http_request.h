#pragma once

#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>
#include <alice/matrix/notificator/library/user_white_list/user_white_list.h>

#include <alice/uniproxy/library/protos/uniproxy.pb.h>

#include <alice/matrix/library/request/http_request.h>


namespace NMatrix::NNotificator {

class TDeliveryHttpRequest : public TProtoHttpRequest<
    NUniproxy::TPushMessage,
    NEvClass::TMatrixNotificatorDeliveryHttpRequestData,
    NEvClass::TMatrixNotificatorDeliveryHttpResponseData
> {
public:
    TDeliveryHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TPushesAndNotificationsClient& pushesAndNotificationsClient,
        const TUserWhiteList& userWhiteList
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    TReply GetReply() const override;

    TString GetReplyData() const;

    NThreading::TFuture<void> OnGetUserSubscriptionsInfo(
        const NThreading::TFuture<TExpected<TPushesAndNotificationsClient::TUserSubscriptionsInfo, TString>>& userSubscriptionsInfoResultFut
    );
    NThreading::TFuture<void> OnAddNotificationResult(
        const NThreading::TFuture<TExpected<TNotificationsStorage::EAddNotificationResult, TString>>& addNotificationResultFut,
        const TPushesAndNotificationsClient::TUserSubscriptionsInfo& userSubscriptionsInfo
    );

public:
    static inline constexpr TStringBuf NAME = "delivery";
    static inline constexpr TStringBuf PATH = "/delivery";

private:
    TPushesAndNotificationsClient& PushesAndNotificationsClient_;
    const TUserWhiteList& UserWhiteList_;

    TNotificationsStorage::TNotification Notification_;

    bool NoLocations_;
    bool NotificationDuplicate_;
    bool UserOrDeviceIsNotSubscribed_;
};

} // namespace NMatrix::NNotificator
