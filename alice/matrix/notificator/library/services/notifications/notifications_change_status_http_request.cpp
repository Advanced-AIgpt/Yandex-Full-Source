#include "notifications_change_status_http_request.h"


namespace NMatrix::NNotificator {

TNotificationsChangeStatusHttpRequest::TNotificationsChangeStatusHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TPushesAndNotificationsClient& pushesAndNotificationsClient
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
{
    if (IsFinished()) {
        return;
    }

    if (Request_->GetPuid().empty()) {
        SetError("Puid is empty", 400);
        IsFinished_ = true;
        return;
    }

    Metrics_.PushRate(Request_->GetNotificationIds().size(), "notification_mark_as_read_count");

    if (Request_->GetNotificationIds().empty()) {
        Metrics_.PushRate("empty_notification_ids_request_count");
        IsFinished_ = true;
        return;
    }
}

NThreading::TFuture<void> TNotificationsChangeStatusHttpRequest::ServeAsync() {
    auto markNotificationsAsReadFut = PushesAndNotificationsClient_.MarkNotificationsAsRead(
        Request_->GetPuid(),
        TVector<TString>(
            Request_->GetNotificationIds().begin(),
            Request_->GetNotificationIds().end()
        ),
        LogContext_,
        Metrics_
    );
    auto userUnsubscribedDevicesFut =  PushesAndNotificationsClient_.GetUserUnsubscribedDevices(
        Request_->GetPuid(),
        LogContext_,
        Metrics_
    );

    return NThreading::WaitAll(
        TVector<NThreading::TFuture<void>>({
            markNotificationsAsReadFut.Apply([](const auto&){}),
            userUnsubscribedDevicesFut.Apply([](const auto&){}),
        })
    ).Apply(
        [this, markNotificationsAsReadFut, userUnsubscribedDevicesFut](const auto&) mutable -> NThreading::TFuture<void> {
            auto markNotificationsAsReadRes = markNotificationsAsReadFut.GetValueSync();
            auto userUnsubscribedDevicesRes = userUnsubscribedDevicesFut.GetValueSync();

            if (!markNotificationsAsReadRes) {
                SetError(markNotificationsAsReadRes.Error(), 500);
                return NThreading::MakeFuture();
            }

            if (!userUnsubscribedDevicesRes) {
                SetError(userUnsubscribedDevicesRes.Error(), 500);
                return NThreading::MakeFuture();
            }
            const auto& userUnsubscribedDevices = userUnsubscribedDevicesRes.Success();

            return PushesAndNotificationsClient_.ActualizeUserNotificationsInfoAndSendItToDevices(
                Request_->GetPuid(),
                /* deviceId = */ Nothing(),
                /* forceNotificationsStateUpdate = */ true,
                /* sendEmptyState = */ false,
                NAlice::NScenarios::TNotifyDirective::NoSound,
                [userUnsubscribedDevices](const TConnectionsStorage::TListConnectionsResult& connections) {
                    THashSet<TStringBuf> userUnsubscribedDevicesSet = THashSet<TStringBuf>(
                        userUnsubscribedDevices.begin(),
                        userUnsubscribedDevices.end()
                    );

                    TConnectionsStorage::TListConnectionsResult filteredResult;
                    for (const auto& connectionRecord : connections.Records) {
                        if (const auto& deviceId = connectionRecord.UserDeviceInfo.DeviceId; !deviceId.empty() && userUnsubscribedDevicesSet.contains(deviceId)) {
                            continue;
                        }

                        filteredResult.Records.emplace_back(connectionRecord);
                    }

                    return filteredResult;
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
                }
            );
        }
    );
}

TNotificationsChangeStatusHttpRequest::TReply TNotificationsChangeStatusHttpRequest::GetReply() const {
    return TReply("", THttpHeaders(), 200);
}

} // namespace NMatrix::NNotificator
