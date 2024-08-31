#include "ydb_metrics_service.h"

#include "ydb_metrics_http_request.h"

namespace NMatrix {

void TYDBMetricsService::OnYDBMetricsHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TYDBMetricsHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request,
        MetricRegistry_
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

bool TYDBMetricsService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    loop.Add(port, TString(TYDBMetricsHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
        OnYDBMetricsHttpRequest(request); 
    });

    // Init metrics
    TSourceMetrics metrics(TYDBMetricsHttpRequest::NAME);
    metrics.InitHttpCode(200, TYDBMetricsHttpRequest::PATH.substr(1));

    return true;
}

} // namespace NMatrix
