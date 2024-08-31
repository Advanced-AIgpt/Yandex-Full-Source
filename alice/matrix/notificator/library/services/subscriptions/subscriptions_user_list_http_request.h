#pragma once

#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>

#include <alice/matrix/library/request/http_request.h>


namespace NMatrix::NNotificator {

class TSubscriptionsUserListHttpRequest : public THttpRequest<
    NEvClass::TMatrixNotificatorSubscriptionsUserListHttpRequestData,
    NEvClass::TMatrixNotificatorSubscriptionsUserListHttpResponseData
> {
public:
    TSubscriptionsUserListHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TPushesAndNotificationsClient& pushesAndNotificationsClient
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    TReply GetReply() const override;

    TString GetReplyData(const TVector<TSubscriptionsStorage::TUserSubscription>& userSubscriptions) const;

public:
    static inline constexpr TStringBuf NAME = "subscriptions_user_list";
    static inline constexpr TStringBuf PATH = "/subscriptions/user_list";

private:
    TPushesAndNotificationsClient& PushesAndNotificationsClient_;

    ui64 SubscriptionId_;
    TMaybe<ui64> AfterTimestamp_;

    TSubscriptionsUserListHttpRequest::TReply HttpReply_;
};

} // namespace NMatrix::NNotificator
