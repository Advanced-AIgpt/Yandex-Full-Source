#include "locator_http_request.h"


namespace NMatrix::NNotificator {

TLocatorHttpRequest::TLocatorHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TLocatorStorage& storage,
    const bool disableYDBOperations
)
    : TProtoHttpRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ false,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            return NNeh::NHttp::ERequestType::Post == method ||
                   NNeh::NHttp::ERequestType::Delete == method;
        }
    )
    , Storage_(storage)
    , DisableYDBOperations_(disableYDBOperations)
{
    if (IsFinished()) {
        return;
    }

    if (DisableYDBOperations_) {
        Metrics_.PushRate("skip_request");
        LogContext_.LogEventInfoCombo<NEvClass::TMatrixNotificatorSkipLocatorRequest>();
        IsFinished_ = true;
        return;
    }
}

NThreading::TFuture<void> TLocatorHttpRequest::ServeAsync() {
    NThreading::TFuture<TExpected<void, TString>> resFut;
    if (NNeh::NHttp::ERequestType::Post == Method_) {
        resFut = Storage_.Store(*Request_, LogContext_, Metrics_);
    } else {
        resFut = Storage_.Remove(*Request_, LogContext_, Metrics_);
    }

    return resFut.Apply(
        [this](const NThreading::TFuture<TExpected<void, TString>>& fut) {
            const auto& res = fut.GetValueSync();
            if (!res) {
                SetError(res.Error(), 500);
            }
        }
    );
}

TLocatorHttpRequest::TReply TLocatorHttpRequest::GetReply() const {
    return TReply(200);
}

} // namespace NMatrix::NNotificator
