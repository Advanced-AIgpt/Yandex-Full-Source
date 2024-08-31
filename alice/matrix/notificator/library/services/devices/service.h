#pragma once

#include <alice/matrix/notificator/library/services/common_context/common_context.h>

#include <alice/matrix/notificator/library/storages/connections/storage.h>
#include <alice/matrix/notificator/library/storages/locator/storage.h>

#include <alice/matrix/library/services/iface/service.h>


namespace NMatrix::NNotificator {

class TDevicesService : public IService {
public:
    explicit TDevicesService(
        const TServicesCommonContext& servicesCommonContext
    );

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    void OnDevicesHttpRequest(const NNeh::IRequestRef& request);

private:
    TLocatorStorage LocatorStorage_;
    TConnectionsStorage ConnectionsStorage_;
    TRtLogClient& RtLogClient_;

    const bool UseOldConnectionsStorage_;
};

} // namespace NMatrix::NNotificator
