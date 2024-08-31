#include "service.h"

#include "proxy_http_request.h"


namespace NMatrix::NNotificator {

namespace {

THolder<TSDClientBase> CreateSDClient(const TProxyServiceSettings& config) {
    if (config.HasAddress()) {
        return MakeHolder<TSDClientDummy>(TString::Join(config.GetAddress().GetHost(), ':', ToString(config.GetAddress().GetPort())));
    } else if (config.HasSD()) {
        Y_ENSURE(!config.GetSD().GetEndpointSetKeys().empty(), "At least one endpoint set key must be provided.");

        return MakeHolder<TSDClient>(config.GetSD());
    } else {
        throw yexception() << "Either Address settings or SD settings must be provided.";
    }
}

TMap<TString, TDuration> GetRouteTimeoutsMap(const TProxyServiceSettings::TTimeoutSettings& timeoutSettings) {
    TMap<TString, TDuration> result;
    for (const auto& routeTimeout : timeoutSettings.GetRouteTimeouts()) {
        Y_ENSURE(!result.contains(routeTimeout.GetRoute()), "Config is invalid: route '" << routeTimeout.GetRoute() << "' occur twice in timeout settings");
        result[routeTimeout.GetRoute()] = FromString<TDuration>(routeTimeout.GetTimeout());
    }

    return result;
}

} // namespace

TProxyService::TProxyService(
    const TServicesCommonContext& servicesCommonContext
)
    : Ncs_()
    , SDClient_(CreateSDClient(servicesCommonContext.Config.GetProxyService()))
    , DefaultTimeout_(FromString<TDuration>(servicesCommonContext.Config.GetProxyService().GetTimeout().GetDefaultTimeout()))
    , RouteTimeouts_(GetRouteTimeoutsMap(servicesCommonContext.Config.GetProxyService().GetTimeout()))
    , DestinationServiceName_(servicesCommonContext.Config.GetProxyService().GetDestinationServiceName())
    , ProxyPingRequest_(servicesCommonContext.Config.GetProxyService().GetProxyPingRequest())
    , RtLogClient_(servicesCommonContext.RtLogClient)
{}

void TProxyService::OnProxyHttpRequest(const NNeh::IRequestRef& request) {
    const auto timeout = GetRequestTimeout(request->Service());
    auto req = MakeIntrusive<TProxyHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request,
        DestinationServiceName_,
        SDClient_,
        timeout,
        Ncs_,
        DeadlineTimersExecutor_.GetIOService()
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

TDuration TProxyService::GetRequestTimeout(const TStringBuf& route) {
    if (const auto ptr = RouteTimeouts_.FindPtr(route)) {
        return *ptr;
    } else {
        return DefaultTimeout_;
    }
}

bool TProxyService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    static const TVector<TString> paths = {
        "/personal_cards/delete",

        "/delivery/sup",
        "/delivery/sup_card",
    };

    TSourceMetrics metrics(TProxyHttpRequest::NAME);
    for (const auto& path : paths) {
        loop.Add(port, path, [this](const NNeh::IRequestRef& request) {
            OnProxyHttpRequest(request);
        });

        // Init sensors with zero.
        metrics.InitHttpCode(200, path.substr(1));
    }

    if (ProxyPingRequest_) {
        static const TString pingPath = "/ping";

        loop.Add(port, pingPath, [this](const NNeh::IRequestRef& request) {
            OnProxyHttpRequest(request);
        });

        // Init sensors with zero.
        metrics.InitHttpCode(200, pingPath.substr(1));
    }

    return true;
}

} // namespace NMatrix::NNotificator
