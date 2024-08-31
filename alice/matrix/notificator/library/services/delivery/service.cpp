#include "service.h"

#include "delivery_demo_http_request.h"
#include "delivery_http_request.h"
#include "delivery_on_connect_http_request.h"
#include "delivery_push_http_request.h"


namespace NMatrix::NNotificator {

TDeliveryService::TDeliveryService(
    const TServicesCommonContext& servicesCommonContext
)
    : PushesAndNotificationsClient_(
        servicesCommonContext.Config.GetPushesAndNotificationsClient(),
        servicesCommonContext.Config.GetSubwayClient(),
        servicesCommonContext.YDBDriver
    )
    , RtLogClient_(servicesCommonContext.RtLogClient)
    , UserWhiteList_(servicesCommonContext.Config.GetUserWhiteList())
{}

void TDeliveryService::OnDeliveryDemoHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TDeliveryDemoHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request,
        PushesAndNotificationsClient_,
        UserWhiteList_
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

void TDeliveryService::OnDeliveryHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TDeliveryHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request,
        PushesAndNotificationsClient_,
        UserWhiteList_
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

void TDeliveryService::OnDeliveryOnConnectHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TDeliveryOnConnectHttpRequest>(
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

void TDeliveryService::OnDeliveryPushHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TDeliveryPushHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request,
        PushesAndNotificationsClient_,
        UserWhiteList_
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

bool TDeliveryService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    {
        loop.Add(port, TString(TDeliveryDemoHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
            OnDeliveryDemoHttpRequest(request);
        });

        // Init metrics
        TSourceMetrics metrics(TDeliveryDemoHttpRequest::NAME);
        metrics.InitHttpCode(200, TDeliveryDemoHttpRequest::PATH.substr(1));
    }

    {
        loop.Add(port, TString(TDeliveryHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
            OnDeliveryHttpRequest(request);
        });

        // Init metrics
        TSourceMetrics metrics(TDeliveryHttpRequest::NAME);
        metrics.InitHttpCode(200, TDeliveryHttpRequest::PATH.substr(1));
    }

    {
        loop.Add(port, TString(TDeliveryOnConnectHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
            OnDeliveryOnConnectHttpRequest(request);
        });

        // Init metrics
        TSourceMetrics metrics(TDeliveryOnConnectHttpRequest::NAME);
        metrics.InitHttpCode(200, TDeliveryOnConnectHttpRequest::PATH.substr(1));
    }

    {
        loop.Add(port, TString(TDeliveryPushHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
            OnDeliveryPushHttpRequest(request);
        });

        // Init metrics
        TSourceMetrics metrics(TDeliveryPushHttpRequest::NAME);
        metrics.InitHttpCode(200, TDeliveryPushHttpRequest::PATH.substr(1));
    }



    return true;
}

} // namespace NMatrix::NNotificator
