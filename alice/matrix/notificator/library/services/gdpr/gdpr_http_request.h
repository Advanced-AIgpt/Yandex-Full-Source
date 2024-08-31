#pragma once

#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>

#include <alice/matrix/library/request/http_request.h>


namespace NMatrix::NNotificator {

class TGDPRHttpRequest : public THttpRequest<
    NEvClass::TMatrixNotificatorGDPRHttpRequestData,
    NEvClass::TMatrixNotificatorGDPRHttpResponseData
> {
public:
    TGDPRHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TPushesAndNotificationsClient& pushesAndNotificationsClient
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    TReply GetReply() const override;

public:
    static inline constexpr TStringBuf NAME = "gdpr";
    static inline constexpr TStringBuf PATH = "/gdpr";

private:
    TPushesAndNotificationsClient& PushesAndNotificationsClient_;

    TString Puid_;
};

} // namespace NMatrix::NNotificator
