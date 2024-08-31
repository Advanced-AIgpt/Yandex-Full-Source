#include "notifications_http_request.h"

#include <alice/matrix/notificator/library/subscriptions_info/subscriptions_info.h>
#include <alice/matrix/notificator/library/utils/utils.h>

#include <library/cpp/cgiparam/cgiparam.h>


namespace NMatrix::NNotificator {

TNotificationsHttpRequest::TNotificationsHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TPushesAndNotificationsClient& pushesAndNotificationsClient
)
    : THttpRequestWithProtoResponse(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ true,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            return NNeh::NHttp::ERequestType::Get == method;
        }
    )
    , PushesAndNotificationsClient_(pushesAndNotificationsClient)

    , Puid_()
    , DeviceId_(Nothing())
    , DeviceModel_(Nothing())
{
    if (IsFinished()) {
        return;
    }

    const TCgiParameters cgi(HttpRequest_->Cgi());
    if (const auto puidIt = cgi.Find("puid"); puidIt != cgi.end()) {
        Puid_ = puidIt->second;
    } else {
        SetError("'puid' param not found", 400);
        IsFinished_ = true;
        return;
    }

    if (const auto deviceIdIt = cgi.Find("device_id"); deviceIdIt != cgi.end()) {
        DeviceId_ = deviceIdIt->second;
    }
    if (const auto deviceModelIt= cgi.Find("device_model"); deviceModelIt != cgi.end()) {
        DeviceModel_ = deviceModelIt->second;
    }

    if (Puid_.empty()) {
        SetError("Puid is empty", 400);
        IsFinished_ = true;
        return;
    }
}

NThreading::TFuture<void> TNotificationsHttpRequest::ServeAsync() {
    return PushesAndNotificationsClient_.GetUserUnsubscribedDevices(Puid_, LogContext_, Metrics_).Apply(
        [this](
            const NThreading::TFuture<TExpected<TVector<TString>, TString>>& fut
        ) {
            const auto& userUnsubscribedDevicesRes = fut.GetValueSync();
            if (SetErrorIfAny(userUnsubscribedDevicesRes)) {
                return NThreading::MakeFuture();
            }

            const auto& userUnsubscribedDevices = userUnsubscribedDevicesRes.Success();
            FillUnsubscribedDevices(userUnsubscribedDevices);

            if (DeviceId_ && Find(userUnsubscribedDevices.begin(), userUnsubscribedDevices.end(), *DeviceId_) != userUnsubscribedDevices.end()) {
                return NThreading::MakeFuture();
            }

            auto userSubscriptionsFut = PushesAndNotificationsClient_.GetUserSubscriptionsForUser(Puid_, LogContext_, Metrics_);
            auto archivedNotificationsCountFut = PushesAndNotificationsClient_.GetArchivedNotificationsCount(Puid_, DeviceId_, LogContext_, Metrics_);
            auto notificationsFut = PushesAndNotificationsClient_.GetNotifications(Puid_, DeviceId_, false /* fromArchive */, LogContext_, Metrics_);
            return NThreading::WaitAll(
                TVector<NThreading::TFuture<void>>({
                    userSubscriptionsFut.Apply([](const auto&){}),
                    archivedNotificationsCountFut.Apply([](const auto&){}),
                    notificationsFut.Apply([](const auto&){})
                })
            ).Apply(
                [
                    this, userSubscriptionsFut = std::move(userSubscriptionsFut), archivedNotificationsCountFut = std::move(archivedNotificationsCountFut), notificationsFut = std::move(notificationsFut)
                ] (const auto&) {
                    auto userSubscriptionsRes = userSubscriptionsFut.GetValueSync();
                    auto archivedNotificationsCountRes = archivedNotificationsCountFut.GetValueSync();
                    auto notificationsRes = notificationsFut.GetValueSync();
                    if (SetErrorIfAny(archivedNotificationsCountRes) || SetErrorIfAny(userSubscriptionsRes) || SetErrorIfAny(notificationsRes)) {
                        return;
                    }

                    auto notifications = notificationsRes.Success();
                    const auto notificationsCount = notifications.size();
                    auto fillNotificationsRes = FillNotifications(std::move(notifications));
                    if (SetErrorIfAny(fillNotificationsRes)) {
                        return;
                    }

                    FillArchivedNotificationsCount(notificationsCount, archivedNotificationsCountRes.Success());
                    FillSubscriptions(userSubscriptionsRes.Success());
                }
            );
        }
    );
}

void TNotificationsHttpRequest::FillUnsubscribedDevices(const TVector<TString>& userUnsubscribedDevices) {
    for (const auto& unsubscribedDevice : userUnsubscribedDevices) {
        Response_.AddUnsubscribedDevices()->SetDeviceId(unsubscribedDevice);
    }
}

TExpected<void, TString> TNotificationsHttpRequest::FillNotifications(
    TVector<TNotificationsStorage::TNotification> notifications
) {
    const auto& subscriptionsInfo = GetSubscriptionsInfo();

    for (auto& notification : notifications) {
        auto subscriptionIdRes = TryParseFromString(notification.Notification.GetSubscriptionId(), "subscription id");
        if (!subscriptionIdRes) {
            return subscriptionIdRes.Error();
        }
        if (subscriptionsInfo.IsDeviceModelSuitableForSubscription(subscriptionIdRes.Success(), DeviceModel_.GetOrElse(""))) {
            *Response_.AddNotifications() = std::move(notification.Notification);
        }
    }

    return TExpected<void, TString>::DefaultSuccess();
}

void TNotificationsHttpRequest::FillArchivedNotificationsCount(
    const ui64 notificationsCount,
    const ui64 archivedNotificationsCount
) {
    if (archivedNotificationsCount > notificationsCount) {
        Response_.SetCountArchived(archivedNotificationsCount - notificationsCount);
    } else {
        Response_.SetCountArchived(0);
    }
}

void TNotificationsHttpRequest::FillSubscriptions(const TMap<ui64, TSubscriptionsStorage::TUserSubscription>& userSubscriptions) {
    const auto& subscriptionsInfo = GetSubscriptionsInfo();

    for (const auto& [subscriptionId, userSubscription] : userSubscriptions) {
        if (const auto& subscriptionInfo = subscriptionsInfo.GetSubscriptions().FindPtr(subscriptionId)) {
            auto* responseSubsription = Response_.MutableSubscriptions()->Add();
            responseSubsription->SetId(ToString(subscriptionId));
            responseSubsription->SetName(subscriptionInfo->GetName());
            responseSubsription->SetTimestamp(ToString(userSubscription.SubscribedAtTimestamp));
        } else {
            // Ignore subscription with removed type
        }
    }
}

} // namespace NMatrix::NNotificator
