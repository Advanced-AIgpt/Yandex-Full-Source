#pragma once

#include <alice/matrix/notificator/library/services/common_context/common_context.h>

#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>
#include <alice/matrix/notificator/library/user_white_list/user_white_list.h>

#include <alice/matrix/library/services/iface/service.h>


namespace NMatrix::NNotificator {

class TDeliveryService : public IService {
public:
    explicit TDeliveryService(
        const TServicesCommonContext& servicesCommonContext
    );

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    void OnDeliveryDemoHttpRequest(const NNeh::IRequestRef& request);
    void OnDeliveryHttpRequest(const NNeh::IRequestRef& request);
    void OnDeliveryOnConnectHttpRequest(const NNeh::IRequestRef& request);
    void OnDeliveryPushHttpRequest(const NNeh::IRequestRef& request);

private:
    TPushesAndNotificationsClient PushesAndNotificationsClient_;
    TRtLogClient& RtLogClient_;
    const TUserWhiteList UserWhiteList_;
};

} // namespace NMatrix::NNotificator
