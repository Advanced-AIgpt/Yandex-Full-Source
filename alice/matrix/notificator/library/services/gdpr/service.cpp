#include "service.h"

#include "gdpr_http_request.h"


namespace NMatrix::NNotificator {

TGDPRService::TGDPRService(
    const TServicesCommonContext& servicesCommonContext
)
    : PushesAndNotificationsClient_(
        servicesCommonContext.Config.GetPushesAndNotificationsClient(),
        servicesCommonContext.Config.GetSubwayClient(),
        servicesCommonContext.YDBDriver
    )
    , RtLogClient_(servicesCommonContext.RtLogClient)
{}

void TGDPRService::OnGDPRHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TGDPRHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request,
        PushesAndNotificationsClient_
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

bool TGDPRService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    loop.Add(port, TString(TGDPRHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
        OnGDPRHttpRequest(request);
    });

    // Init metrics
    TSourceMetrics metrics(TGDPRHttpRequest::NAME);
    metrics.InitHttpCode(200, TGDPRHttpRequest::PATH.substr(1));

    return true;
}

} // namespace NMatrix::NNotificator
