#pragma once

#include <alice/matrix/notificator/library/services/common_context/common_context.h>
#include <alice/matrix/notificator/library/storages/locator/storage.h>

#include <alice/matrix/library/services/iface/service.h>

namespace NMatrix::NNotificator {

class TLocatorService : public IService {
public:
    explicit TLocatorService(
        const TServicesCommonContext& servicesCommonContext
    );

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    void OnLocatorHttpRequest(const NNeh::IRequestRef& request);

private:
    TLocatorStorage LocatorStorage_;
    TRtLogClient& RtLogClient_;

    const bool DisableYDBOperations_;
};

} // namespace NMatrix::NNotificator
