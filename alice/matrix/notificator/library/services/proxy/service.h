#pragma once

#include "sd_client.h"

#include <alice/matrix/notificator/library/services/common_context/common_context.h>

#include <alice/matrix/library/metrics/metrics.h>
#include <alice/matrix/library/rtlog/rtlog.h>
#include <alice/matrix/library/services/iface/service.h>

#include <apphost/lib/transport/transport_neh_backend.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>

#include <library/cpp/neh/asio/executor.h>


namespace NMatrix::NNotificator {

class TProxyService : public IService {
public:
    explicit TProxyService(
        const TServicesCommonContext& servicesCommonContext
    );

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    void OnProxyHttpRequest(const NNeh::IRequestRef& request);

    TDuration GetRequestTimeout(const TStringBuf& route);

private:
    NAppHost::NTransport::TNehCommunicationSystem Ncs_;
    THolder<TSDClientBase> SDClient_;
    const TDuration DefaultTimeout_;
    // Map route -> timeout
    const TMap<TString, TDuration> RouteTimeouts_;
    const TString DestinationServiceName_;
    const bool ProxyPingRequest_;

    TRtLogClient& RtLogClient_;

    NAsio::TIOServiceExecutor DeadlineTimersExecutor_;
};

} // namespace NMatrix::NNotificator
