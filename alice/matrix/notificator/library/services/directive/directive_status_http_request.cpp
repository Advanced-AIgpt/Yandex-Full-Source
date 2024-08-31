#include "directive_status_http_request.h"


namespace NMatrix::NNotificator {

TDirectiveStatusHttpRequest::TDirectiveStatusHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TDirectivesStorage& directivesStorage
)
    : THttpRequestWithProtoResponse(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ false,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            return NNeh::NHttp::ERequestType::Post == method;
        }
    )
    , DirectivesStorage_(directivesStorage)
{}

NThreading::TFuture<void> TDirectiveStatusHttpRequest::ServeAsync() {
    return DirectivesStorage_.GetDirectiveStatus(*Request_, LogContext_, Metrics_).Apply(
        [this](const NThreading::TFuture<TExpected<NAlice::NNotificator::EDirectiveStatus, TString>>& fut) {
            const auto& res = fut.GetValueSync();
            if (!res) {
                SetError(res.Error(), 500);
                return;
            }
            Response_.SetStatus(res.Success());
        }
    );
}

} // namespace NMatrix::NNotificator
