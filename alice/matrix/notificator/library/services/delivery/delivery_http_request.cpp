#include "delivery_http_request.h"

#include <alice/matrix/notificator/library/subscriptions_info/subscriptions_info.h>
#include <alice/matrix/notificator/library/utils/utils.h>

#include <util/generic/guid.h>


namespace NMatrix::NNotificator {

namespace {

using std::placeholders::_1;

} // namespace

TDeliveryHttpRequest::TDeliveryHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TPushesAndNotificationsClient& pushesAndNotificationsClient,
    const TUserWhiteList& userWhiteList
)
    : TProtoHttpRequest(
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
    , NoLocations_(false)
    , NotificationDuplicate_(false)
    , UserOrDeviceIsNotSubscribed_(false)
{
    if (IsFinished()) {
        return;
    }

    if (Request_->GetUid().empty()) {
        SetError("Puid is empty", 400);
        IsFinished_ = true;
        return;
    }

    if (Request_->HasNotification()) {
        {
            // Some legacy logic from python

            // Always override id
            Request_->MutableNotification()->SetId(TGUID::Create().AsUuidString());

            // Move subscription id to correct place
            Request_->MutableNotification()->SetSubscriptionId(ToString(Request_->GetSubscriptionId()));
        }

        Notification_ = TNotificationsStorage::TNotification({
            .DeviceId = Request_->GetDeviceId().empty() ? Nothing() : TMaybe<TString>(Request_->GetDeviceId()),
            .Notification = Request_->GetNotification(),
        });
    } else {
        SetError("Notification content is empty", 400);
        IsFinished_ = true;
        return;
    }

    if (!UserWhiteList_.IsPuidAllowedToProcess(Request_->GetUid())) {
        SetError("Puid from request is not in white list", 403);
        IsFinished_ = true;
        return;
    }
}

NThreading::TFuture<void> TDeliveryHttpRequest::ServeAsync() {
    return PushesAndNotificationsClient_.GetUserSubscriptionsInfo(
        Request_->GetUid(),
        LogContext_,
        Metrics_
    ).Apply(
        std::bind(&TDeliveryHttpRequest::OnGetUserSubscriptionsInfo, this, _1)
    );
}

TDeliveryHttpRequest::TReply TDeliveryHttpRequest::GetReply() const {
    return TReply(GetReplyData(), THttpHeaders(), NotificationDuplicate_ ? 208 : 200);
}

TString TDeliveryHttpRequest::GetReplyData() const {
    if (NoLocations_) {
        return R"({"code": 404, "error": "No locations at all"})";
    } else if (NotificationDuplicate_) {
        return R"({"code": 208, "error": "Notification duplicate"})";
    } else if (UserOrDeviceIsNotSubscribed_) {
        return R"({"code": 404, "error": "User or device is not subscribed"})";
    } else {
        return TString::Join(R"({"code": 200, "id": ")", Notification_.Notification.GetId(), R"("})");
    }
}

NThreading::TFuture<void> TDeliveryHttpRequest::OnGetUserSubscriptionsInfo(
    const NThreading::TFuture<TExpected<TPushesAndNotificationsClient::TUserSubscriptionsInfo, TString>>& userSubscriptionsInfoResultFut
) {
    auto userSubscriptionsInfoResult = userSubscriptionsInfoResultFut.GetValueSync();
    if (!userSubscriptionsInfoResult) {
        SetError(userSubscriptionsInfoResult.Error(), 500);
        return NThreading::MakeFuture();
    }
    const auto& userSubscriptionsInfo = userSubscriptionsInfoResult.Success();

    if (
        !userSubscriptionsInfo.IsUserSubscribedToSubscription(static_cast<ui64>(Max<i64>(0, Request_->GetSubscriptionId()))) ||
        (Notification_.DeviceId.Defined() && !userSubscriptionsInfo.IsDeviceSubscribed(*Notification_.DeviceId))
    ) {
        Metrics_.PushRate("not_subscribed");
        UserOrDeviceIsNotSubscribed_ = true;
        return NThreading::MakeFuture();
    }

    return PushesAndNotificationsClient_.AddNotification(
        Request_->GetUid(),
        Notification_,
        /* allowNotificationDuplicates = */ false,
        LogContext_,
        Metrics_
    ).Apply(
        std::bind(&TDeliveryHttpRequest::OnAddNotificationResult, this, _1, userSubscriptionsInfo)
    );
}

NThreading::TFuture<void> TDeliveryHttpRequest::OnAddNotificationResult(
    const NThreading::TFuture<TExpected<TNotificationsStorage::EAddNotificationResult, TString>>& addNotificationResultFut,
    const TPushesAndNotificationsClient::TUserSubscriptionsInfo& userSubscriptionsInfo
) {
    const auto& addNotificationResult = addNotificationResultFut.GetValueSync();
    if (addNotificationResult.IsError()) {
        SetError(addNotificationResult.Error(), 500);
        return NThreading::MakeFuture();
    }
    const auto& addNotificationSuccessResult = addNotificationResult.Success();

    if (addNotificationSuccessResult == TNotificationsStorage::EAddNotificationResult::ALREADY_EXIST) {
        Metrics_.PushRate("notification_duplicate");
        NotificationDuplicate_ = true;
        return NThreading::MakeFuture();
    }

    return PushesAndNotificationsClient_.ActualizeUserNotificationsInfoAndSendItToDevices(
        Request_->GetUid(),
        Request_->GetDeviceId().empty() ? Nothing() : TMaybe<TString>(Request_->GetDeviceId()),
        /* forceNotificationsStateUpdate = */ true,
        /* sendEmptyState = */ false,
        static_cast<NAlice::NScenarios::TNotifyDirective::ERingType>(Request_->GetRing()),
        [subscriptionId = Request_->GetSubscriptionId(), unsubscribedDevices = userSubscriptionsInfo.UnsubscribedDevices](const TConnectionsStorage::TListConnectionsResult& connections) {
            return GetSubscriptionsInfo().FilterListConnectionsResultBySubscriptionAndUnsubscribedDevices(
                connections,
                subscriptionId,
                unsubscribedDevices
            );
        },
        LogContext_,
        Metrics_
    ).Apply(
        [this](const NThreading::TFuture<TExpected<TVector<NUniproxy::TSubwayResponse>, TString>>& fut) mutable {
            const auto& futRes = fut.GetValueSync();
            if (!futRes) {
                SetError(futRes.Error(), 500);
                return;
            }

            if (futRes.Success().empty()) {
                Metrics_.PushRate("no_locations");
                NoLocations_ = true;
            }
        }
    );
}

} // namespace NMatrix::NNotificator
