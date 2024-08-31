#pragma once

#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>

#include <alice/matrix/library/clients/iot_client/iot_client.h>
#include <alice/matrix/library/request/http_request.h>


namespace NMatrix::NNotificator {

class TSubscriptionsDevicesHttpRequest : public THttpRequest<
    // This request is bad
    // It combines two completely different logics (get device subscriptions and manage device subscriptions)
    // and selects the mode depending on the parameters

    /* TRequestDataEvent = */ NPrivate::TFakeDataEvent,
    NEvClass::TMatrixNotificatorSubscriptionsDevicesHttpResponseData,
    /* RequestDataEventPatcher = */ EmptyRequestEventPatcher<NPrivate::TFakeDataEvent>,
    EmptyRequestEventPatcher<NEvClass::TMatrixNotificatorSubscriptionsDevicesHttpResponseData>,

    // Do not log raw request data
    // Special logging is implemented in this class
    /* LogRawRequestData = */ false,

    /* LogRawResponseData = */ true
> {
public:
    TSubscriptionsDevicesHttpRequest(
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

    NThreading::TFuture<void> ProcessGetRequest();
    NThreading::TFuture<void> ProcessManageRequest();

    TString GetReplyDataForGetRequest(
        const TVector<TString>& userUnsubscribedDevices,
        const NAlice::TIoTUserInfo& iotUserInfo
    ) const;
    TString GetReplyDataForManageRequest();

public:
    static inline constexpr TStringBuf NAME = "subscriptions_devices";
    static inline constexpr TStringBuf PATH = "/subscriptions/devices";

private:
    enum class ESubscriptionMethod {
        SUBSCRIBE = 0,
        UNSUBSCRIBE = 1,
    };

private:
    TIoTClient& IoTClient_;
    TPushesAndNotificationsClient& PushesAndNotificationsClient_;

    TString UserTicket_;
    TString Puid_;
    TString DeviceId_;
    ESubscriptionMethod SubscriptionMethod_;

    TSubscriptionsDevicesHttpRequest::TReply HttpReply_;
};

} // namespace NMatrix::NNotificator
