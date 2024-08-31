#pragma once
#include <alice/matrix/library/rtlog/rtlog.h>

#include <alice/matrix/library/services/iface/service.h>

namespace NMatrix {

class TMainMetricsService : public IService {
public:
    template <typename TServicesCommonContext>
    explicit TMainMetricsService(
        const TServicesCommonContext& servicesCommonContext
    )
        : RtLogClient_(servicesCommonContext.RtLogClient)
    {}

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    void OnMainMetricsHttpRequest(const NNeh::IRequestRef& request);

private:
    TRtLogClient& RtLogClient_;
};

} // namespace NMatrix