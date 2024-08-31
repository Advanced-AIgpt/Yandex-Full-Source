#include "service.h"

#include "devices_http_request.h"

namespace NMatrix::NNotificator {

TDevicesService::TDevicesService(
    const TServicesCommonContext& servicesCommonContext
)
    : LocatorStorage_(
        servicesCommonContext.YDBDriver,
        servicesCommonContext.Config.GetDevicesService().GetYDBClient()
    )
    , ConnectionsStorage_(
        servicesCommonContext.YDBDriver,
        servicesCommonContext.Config.GetDevicesService().GetYDBClient()
    )
    , RtLogClient_(servicesCommonContext.RtLogClient)
    , UseOldConnectionsStorage_(servicesCommonContext.Config.GetDevicesService().GetUseOldConnectionsStorage())
{}

void TDevicesService::OnDevicesHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TDevicesHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request,
        LocatorStorage_,
        ConnectionsStorage_,
        UseOldConnectionsStorage_
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

bool TDevicesService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    loop.Add(port, TString(TDevicesHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
        OnDevicesHttpRequest(request);
    });

    // Init metrics
    TSourceMetrics metrics(TDevicesHttpRequest::NAME);
    metrics.InitHttpCode(200, TDevicesHttpRequest::PATH.substr(1));

    return true;
}

} // namespace NMatrix::NNotificator
