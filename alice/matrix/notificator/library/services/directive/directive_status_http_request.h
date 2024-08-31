#pragma once

#include <alice/matrix/notificator/library/storages/directives/storage.h>

#include <alice/matrix/library/request/http_request.h>


namespace NMatrix::NNotificator {

class TDirectiveStatusHttpRequest : public TProtoHttpRequestWithProtoResponse<
    NAlice::NNotificator::TDirectiveStatus,
    NAlice::NNotificator::TDirectiveStatusResponse,
    NEvClass::TMatrixNotificatorDirectiveStatusHttpRequestData,
    NEvClass::TMatrixNotificatorDirectiveStatusHttpResponseData
> {
public:
    TDirectiveStatusHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TDirectivesStorage& directivesStorage
    );

    NThreading::TFuture<void> ServeAsync() override;

public:
    static inline constexpr TStringBuf NAME = "directive_status";
    static inline constexpr TStringBuf PATH = "/directive/status";

private:
    TDirectivesStorage& DirectivesStorage_;
};

} // namespace NMatrix::NNotificator
