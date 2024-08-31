#pragma once

#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>

#include <alice/matrix/library/request/http_request.h>

#include <alice/uniproxy/library/protos/notificator.pb.h>


namespace NMatrix::NNotificator {

class TNotificationsChangeStatusHttpRequest : public TProtoHttpRequest<
    ::NNotificator::TNotificationChangeStatus,
    NEvClass::TMatrixNotificatorNotificationsChangeStatusHttpRequestData,
    NEvClass::TMatrixNotificatorNotificationsChangeStatusHttpResponseData
> {
public:
    TNotificationsChangeStatusHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TPushesAndNotificationsClient& pushesAndNotificationsClient
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    TReply GetReply() const override;

public:
    static inline constexpr TStringBuf NAME = "notifications_change_status";
    static inline constexpr TStringBuf PATH = "/notifications/change_status";

private:
    TPushesAndNotificationsClient& PushesAndNotificationsClient_;
};

} // namespace NMatrix::NNotificator
