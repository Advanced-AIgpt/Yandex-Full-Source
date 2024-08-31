#include "service.h"

#include "notifications_change_status_http_request.h"
#include "notifications_http_request.h"


namespace NMatrix::NNotificator {

TNotificationsService::TNotificationsService(
    const TServicesCommonContext& servicesCommonContext
)
    : PushesAndNotificationsClient_(
        servicesCommonContext.Config.GetPushesAndNotificationsClient(),
        servicesCommonContext.Config.GetSubwayClient(),
        servicesCommonContext.YDBDriver
    )
    , RtLogClient_(servicesCommonContext.RtLogClient)
{}

void TNotificationsService::OnNotificationsChangeStatusHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TNotificationsChangeStatusHttpRequest>(
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

void TNotificationsService::OnNotificationsHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TNotificationsHttpRequest>(
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

bool TNotificationsService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    {
        loop.Add(port, TString(TNotificationsChangeStatusHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
            OnNotificationsChangeStatusHttpRequest(request);
        });

        // Init metrics
        TSourceMetrics metrics(TNotificationsChangeStatusHttpRequest::NAME);
        metrics.InitHttpCode(200, TNotificationsChangeStatusHttpRequest::PATH.substr(1));
    }

    {
        loop.Add(port, TString(TNotificationsHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
            OnNotificationsHttpRequest(request);
        });

        // Init metrics
        TSourceMetrics metrics(TNotificationsHttpRequest::NAME);
        metrics.InitHttpCode(200, TNotificationsHttpRequest::PATH.substr(1));
    }

    return true;
}

} // namespace NMatrix::NNotificator
