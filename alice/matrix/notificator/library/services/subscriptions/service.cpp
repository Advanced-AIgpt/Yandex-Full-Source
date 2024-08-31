#include "service.h"

#include "subscriptions_devices_http_request.h"
#include "subscriptions_http_request.h"
#include "subscriptions_manage_http_request.h"
#include "subscriptions_user_list_http_request.h"


namespace NMatrix::NNotificator {

TSubscriptionsService::TSubscriptionsService(
    const TServicesCommonContext& servicesCommonContext
)
    : IoTClient_(
        servicesCommonContext.Config.GetIoTClient(),
        servicesCommonContext.TvmClient
    )
    , PushesAndNotificationsClient_(
        servicesCommonContext.Config.GetPushesAndNotificationsClient(),
        servicesCommonContext.Config.GetSubwayClient(),
        servicesCommonContext.YDBDriver
    )
    , TvmClient_(servicesCommonContext.TvmClient)
    , RtLogClient_(servicesCommonContext.RtLogClient)
    , UserWhiteList_(servicesCommonContext.Config.GetUserWhiteList())
{}

void TSubscriptionsService::OnSubscriptionsDevicesHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TSubscriptionsDevicesHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request,
        IoTClient_,
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

    if (req->NeedTvmServiceTicket() && !TvmClient_.EnsureInitializedAndReady()) {
        req->SetError("Service is not ready (bad tvmtool response)", 503);
        req->Reply();
        return;
    }

    req->ServeAsync().Apply([req](const NThreading::TFuture<void>& fut) {
        return req->ReplyWithFutureCheck(fut);
    });
}

void TSubscriptionsService::OnSubscriptionsHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TSubscriptionsHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request,
        IoTClient_,
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

    if (req->NeedTvmServiceTicket() && !TvmClient_.EnsureInitializedAndReady()) {
        req->SetError("Service is not ready (bad tvmtool response)", 503);
        req->Reply();
        return;
    }

    req->ServeAsync().Apply([req](const NThreading::TFuture<void>& fut) {
        return req->ReplyWithFutureCheck(fut);
    });
}

void TSubscriptionsService::OnSubscriptionsManageHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TSubscriptionsManageHttpRequest>(
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

void TSubscriptionsService::OnSubscriptionsUserListHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TSubscriptionsUserListHttpRequest>(
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

bool TSubscriptionsService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    {
        loop.Add(port, TString(TSubscriptionsDevicesHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
            OnSubscriptionsDevicesHttpRequest(request);
        });

        // Init metrics
        TSourceMetrics metrics(TSubscriptionsDevicesHttpRequest::NAME);
        metrics.InitHttpCode(200, TSubscriptionsDevicesHttpRequest::PATH.substr(1));
    }

    {
        loop.Add(port, TString(TSubscriptionsHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
            OnSubscriptionsHttpRequest(request);
        });

        // Init metrics
        TSourceMetrics metrics(TSubscriptionsHttpRequest::NAME);
        metrics.InitHttpCode(200, TSubscriptionsHttpRequest::PATH.substr(1));
    }

    {
        loop.Add(port, TString(TSubscriptionsManageHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
            OnSubscriptionsManageHttpRequest(request);
        });

        // Init metrics
        TSourceMetrics metrics(TSubscriptionsManageHttpRequest::NAME);
        metrics.InitHttpCode(200, TSubscriptionsManageHttpRequest::PATH.substr(1));
    }

    {
        loop.Add(port, TString(TSubscriptionsUserListHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
            OnSubscriptionsUserListHttpRequest(request);
        });

        // Init metrics
        TSourceMetrics metrics(TSubscriptionsUserListHttpRequest::NAME);
        metrics.InitHttpCode(200, TSubscriptionsUserListHttpRequest::PATH.substr(1));
    }

    return true;
}

} // namespace NMatrix::NNotificator
