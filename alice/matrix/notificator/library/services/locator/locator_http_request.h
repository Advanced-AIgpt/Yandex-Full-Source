#pragma once

#include <alice/matrix/notificator/library/storages/locator/storage.h>

#include <alice/matrix/library/request/http_request.h>

namespace NMatrix::NNotificator {

class TLocatorHttpRequest : public TProtoHttpRequest<
    ::NNotificator::TDeviceLocator,
    NEvClass::TMatrixNotificatorLocatorHttpRequestData,
    NEvClass::TMatrixNotificatorLocatorHttpResponseData
> {
public:
    TLocatorHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TLocatorStorage& storage,
        const bool disableYDBOperations
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    TReply GetReply() const override;

public:
    static inline constexpr TStringBuf NAME = "locator";
    static inline constexpr TStringBuf PATH = "/locator";

private:
    TLocatorStorage& Storage_;

    const bool DisableYDBOperations_;
};

} // namespace NMatrix::NNotificator
