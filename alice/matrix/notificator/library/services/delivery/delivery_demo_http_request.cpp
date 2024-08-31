#include "delivery_demo_http_request.h"

#include <alice/matrix/notificator/library/subscriptions_info/subscriptions_info.h>
#include <alice/matrix/notificator/library/utils/utils.h>

#include <library/cpp/cgiparam/cgiparam.h>


namespace NMatrix::NNotificator {

TDeliveryDemoHttpRequest::TDeliveryDemoHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TPushesAndNotificationsClient& pushesAndNotificationsClient,
    const TUserWhiteList& userWhiteList
)
    : THttpRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ true,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            // It should be a post request, but for historical reasons it is a get
            // https://a.yandex-team.ru/svn/trunk/arcadia/alice/uniproxy/library/notificator/delivery.py?rev=r9184978#L657
            return NNeh::NHttp::ERequestType::Get == method;
        }
    )
    , PushesAndNotificationsClient_(pushesAndNotificationsClient)
    , UserWhiteList_(userWhiteList)

    , Puid_()
    , Notification_()
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

    if (const auto subscriptionIdIt = cgi.Find("subscription_id"); subscriptionIdIt != cgi.end()) {
        if (auto parseResult = TryParseFromString(subscriptionIdIt->second, "subscription id")) {
            SubscriptionId_ = parseResult.Success();
        } else {
            SetError(parseResult.Error(), 400);
            IsFinished_ = true;
            return;
        }
    } else {
        SetError("'subscription_id' param not found", 400);
        IsFinished_ = true;
        return;
    }

    if (Puid_.empty()) {
        SetError("Puid is empty", 400);
        IsFinished_ = true;
        return;
    }

    if (const auto* subscriptionInfo = GetSubscriptionsInfo().GetSubscriptions().FindPtr(SubscriptionId_)) {
        if (subscriptionInfo->HasDemo()) {
            Notification_ = TNotificationsStorage::TNotification({
                .DeviceId = Nothing(),
                .Notification = subscriptionInfo->GetDemo(),
            });

            Notification_.Notification.SetId(TGUID::Create().AsUuidString());
            Notification_.Notification.SetSubscriptionId(ToString(SubscriptionId_));
        } else {
            SetError("Subscription does not have a demo notification", 400);
            IsFinished_ = true;
            return;
        }
    } else {
        SetError("Unknown subscription id", 400);
        IsFinished_ = true;
        return;
    }

    if (!UserWhiteList_.IsPuidAllowedToProcess(Puid_)) {
        SetError("Puid from request is not in white list", 403);
        IsFinished_ = true;
        return;
    }
}

NThreading::TFuture<void> TDeliveryDemoHttpRequest::ServeAsync() {
    auto addNotificationFut = PushesAndNotificationsClient_.AddNotification(
        Puid_,
        Notification_,
        // It is ok to have two or more same demo notifications
        /* allowNotificationDuplicates = */ true,
        LogContext_,
        Metrics_
    );
    auto userUnsubscribedDevicesFut =  PushesAndNotificationsClient_.GetUserUnsubscribedDevices(
        Puid_,
        LogContext_,
        Metrics_
    );

    return NThreading::WaitAll(
        TVector<NThreading::TFuture<void>>({
            addNotificationFut.Apply([](const auto&){}),
            userUnsubscribedDevicesFut.Apply([](const auto&){}),
        })
    ).Apply(
        [this, addNotificationFut, userUnsubscribedDevicesFut](const auto&) mutable -> NThreading::TFuture<void> {
            auto addNotificationRes = addNotificationFut.GetValueSync();
            auto userUnsubscribedDevicesRes = userUnsubscribedDevicesFut.GetValueSync();

            if (!addNotificationRes) {
                SetError(addNotificationRes.Error(), 500);
                return NThreading::MakeFuture();
            }

            if (!userUnsubscribedDevicesRes) {
                SetError(userUnsubscribedDevicesRes.Error(), 500);
                return NThreading::MakeFuture();
            }

            return PushesAndNotificationsClient_.ActualizeUserNotificationsInfoAndSendItToDevices(
                Puid_,
                /* deviceId = */ Nothing(),
                /* forceNotificationsStateUpdate = */ true,
                /* sendEmptyState = */ false,
                NAlice::NScenarios::TNotifyDirective::Delicate,
                [subscriptionId = SubscriptionId_, userUnsubscribedDevices = userUnsubscribedDevicesRes.Success()](const TConnectionsStorage::TListConnectionsResult& connections) {
                    return GetSubscriptionsInfo().FilterListConnectionsResultBySubscriptionAndUnsubscribedDevices(
                        connections,
                        subscriptionId,
                        userUnsubscribedDevices
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
                    bool noLocations = futRes.Success().empty();

                    if (noLocations) {
                        Metrics_.PushRate("no_locations");
                    }
                    HttpReply_ = TReply(GetReplyData(noLocations), THttpHeaders(), 200);
                }
            );
        }
    );
}

TDeliveryDemoHttpRequest::TReply TDeliveryDemoHttpRequest::GetReply() const {
    return HttpReply_;
}

TString TDeliveryDemoHttpRequest::GetReplyData(bool noLocations) const {
    if (noLocations) {
        return R"({"code": 404, "error": "No locations at all"})";
    } else {
        return TString::Join(R"({"code": 200, "id": ")", Notification_.Notification.GetId(), R"("})");
    }
}

} // namespace NMatrix::NNotificator
