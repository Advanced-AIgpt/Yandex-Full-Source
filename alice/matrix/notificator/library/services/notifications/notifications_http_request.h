#pragma once

#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>

#include <alice/matrix/library/request/http_request.h>


namespace NMatrix::NNotificator {

class TNotificationsHttpRequest : public TRawHttpRequestWithProtoResponse<
    NAlice::TNotificationState,
    NEvClass::TMatrixNotificatorNotificationsHttpRequestData,
    NEvClass::TMatrixNotificatorNotificationsHttpResponseData
> {
public:
    TNotificationsHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TPushesAndNotificationsClient& pushesAndNotificationsClient
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    TExpected<void, TString> FillNotifications(
        TVector<TNotificationsStorage::TNotification> notifications
    );

    void FillUnsubscribedDevices(const TVector<TString>& userUnsubscribedDevices);
    void FillArchivedNotificationsCount(
        const ui64 notificationsCount,
        const ui64 archivedNotificationsCount
    );
    void FillSubscriptions(const TMap<ui64, TSubscriptionsStorage::TUserSubscription>& userSubscriptions);

    template <typename T>
    bool SetErrorIfAny(const T& res) {
        if (res.IsError()) {
            SetError(res.Error(), 500);
            return true;
        }

        return false;
    }

public:
    static inline constexpr TStringBuf NAME = "notifications";
    static inline constexpr TStringBuf PATH = "/notifications";

private:
    TPushesAndNotificationsClient& PushesAndNotificationsClient_;

    TString Puid_;
    TMaybe<TString> DeviceId_;
    TMaybe<TString> DeviceModel_;
};

} // namespace NMatrix::NNotificator
