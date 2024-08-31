#pragma once

#include <alice/matrix/library/rtlog/rtlog.h>

#include <alice/matrix/library/services/iface/service.h>

#include <ydb/public/sdk/cpp/client/extensions/solomon_stats/pull_connector.h>

#include <library/cpp/monlib/metrics/metric_registry.h>

namespace NMatrix {

class TYDBMetricsService : public IService {
public:
    template <typename TServicesCommonContext>
    explicit TYDBMetricsService(
        const TServicesCommonContext& servicesCommonContext
    )
        : RtLogClient_(servicesCommonContext.RtLogClient)
        , MetricRegistry_()
    {
        NSolomonStatExtension::AddMetricRegistry(servicesCommonContext.YDBDriver, &MetricRegistry_);
    }

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    void OnYDBMetricsHttpRequest(const NNeh::IRequestRef& request);

private:
    TRtLogClient& RtLogClient_;
    NMonitoring::TMetricRegistry MetricRegistry_;
};

} // namespace NMatrix