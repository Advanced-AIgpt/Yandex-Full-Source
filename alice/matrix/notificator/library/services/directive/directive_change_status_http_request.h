#pragma once

#include <alice/matrix/notificator/library/storages/directives/storage.h>

#include <alice/matrix/library/request/http_request.h>

namespace NMatrix::NNotificator {

class TDirectiveChangeStatusHttpRequest : public TProtoHttpRequest<
    NAlice::NNotificator::TChangeStatus,
    NEvClass::TMatrixNotificatorDirectiveChangeStatusHttpRequestData,
    NEvClass::TMatrixNotificatorDirectiveChangeStatusHttpResponseData
> {
public:
    TDirectiveChangeStatusHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TDirectivesStorage& directivesStorage
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    TReply GetReply() const override;

public:
    static inline constexpr TStringBuf NAME = "directive_change_status";
    static inline constexpr TStringBuf PATH = "/directive/change_status";

private:
    TDirectivesStorage& DirectivesStorage_;
};

} // namespace NMatrix::NNotificator
