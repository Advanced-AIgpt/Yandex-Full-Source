#include "service.h"

#include "directive_change_status_http_request.h"
#include "directive_status_http_request.h"


namespace NMatrix::NNotificator {

TDirectiveService::TDirectiveService(
    const TServicesCommonContext& servicesCommonContext
)
    : DirectivesStorage_(
        servicesCommonContext.YDBDriver,
        servicesCommonContext.Config.GetDirectiveService().GetYDBClient()
    )
    , RtLogClient_(servicesCommonContext.RtLogClient)
{}

void TDirectiveService::OnDirectiveChangeStatusHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TDirectiveChangeStatusHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request,
        DirectivesStorage_
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

void TDirectiveService::OnDirectiveStatusHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TDirectiveStatusHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request,
        DirectivesStorage_
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

bool TDirectiveService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    {
        loop.Add(port, TString(TDirectiveChangeStatusHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
            OnDirectiveChangeStatusHttpRequest(request);
        });

        // Init metrics
        TSourceMetrics metrics(TDirectiveChangeStatusHttpRequest::NAME);
        metrics.InitHttpCode(200, TDirectiveChangeStatusHttpRequest::PATH.substr(1));
    }

    {
        loop.Add(port, TString(TDirectiveStatusHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
            OnDirectiveStatusHttpRequest(request);
        });

        // Init metrics
        TSourceMetrics metrics(TDirectiveStatusHttpRequest::NAME);
        metrics.InitHttpCode(200, TDirectiveStatusHttpRequest::PATH.substr(1));
    }

    return true;
}

} // namespace NMatrix::NNotificator
