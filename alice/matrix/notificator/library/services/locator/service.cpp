#include "service.h"

#include "locator_http_request.h"


namespace NMatrix::NNotificator {

TLocatorService::TLocatorService(
    const TServicesCommonContext& servicesCommonContext
)
    : LocatorStorage_(
        servicesCommonContext.YDBDriver,
        servicesCommonContext.Config.GetLocatorService().GetYDBClient()
    )
    , RtLogClient_(servicesCommonContext.RtLogClient)
    , DisableYDBOperations_(servicesCommonContext.Config.GetLocatorService().GetDisableYDBOperations())
{}

void TLocatorService::OnLocatorHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TLocatorHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request,
        LocatorStorage_,
        DisableYDBOperations_
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

    req->ServeAsync().Apply([req](const NThreading::TFuture<void>& fut) {
        return req->ReplyWithFutureCheck(fut);
    });
}

bool TLocatorService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    loop.Add(port, TString(TLocatorHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
        OnLocatorHttpRequest(request);
    });

    // Init metrics
    TSourceMetrics metrics(TLocatorHttpRequest::NAME);
    metrics.InitHttpCode(200, TLocatorHttpRequest::PATH.substr(1));

    return true;
}

} // namespace NMatrix::NNotificator
