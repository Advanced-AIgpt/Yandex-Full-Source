#pragma once

#include <alice/matrix/notificator/library/services/common_context/common_context.h>

#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>

#include <alice/matrix/library/services/iface/service.h>


namespace NMatrix::NNotificator {

class TGDPRService : public IService {
public:
    explicit TGDPRService(
        const TServicesCommonContext& servicesCommonContext
    );

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    void OnGDPRHttpRequest(const NNeh::IRequestRef& request);

private:
    TPushesAndNotificationsClient PushesAndNotificationsClient_;
    TRtLogClient& RtLogClient_;
};

} // namespace NMatrix::NNotificator
