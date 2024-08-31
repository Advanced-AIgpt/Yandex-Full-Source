#include "main_metrics_service.h"

#include <alice/matrix/library/services/metrics/main_metrics_http_request.h>

namespace NMatrix {

void TMainMetricsService::OnMainMetricsHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TMainMetricsHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request
    );

    if (req->IsFinished()) {
        req->Reply();
        return;
    }

    if (IsSuspended()) {
        req->SetError("Service is suspended", 503);
        req->Reply();
        return;
    }

    req->ServeAsync().Apply(
        [req](const NThreading::TFuture<void>& fut) {
            return req->ReplyWithFutureCheck(fut);
        }
    );
}

bool TMainMetricsService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    loop.Add(port, TString(TMainMetricsHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
        OnMainMetricsHttpRequest(request); 
    });

    // Init metrics
    TSourceMetrics metrics(TMainMetricsHttpRequest::NAME);
    metrics.InitHttpCode(200, TMainMetricsHttpRequest::PATH.substr(1));

    return true;
}

} // namespace NMatrix
