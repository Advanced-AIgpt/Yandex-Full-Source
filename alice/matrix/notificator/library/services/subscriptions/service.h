#pragma once

#include <alice/matrix/notificator/library/services/common_context/common_context.h>

#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>
#include <alice/matrix/notificator/library/user_white_list/user_white_list.h>

#include <alice/matrix/library/clients/iot_client/iot_client.h>
#include <alice/matrix/library/clients/tvm_client/tvm_client.h>
#include <alice/matrix/library/services/iface/service.h>


namespace NMatrix::NNotificator {

class TSubscriptionsService : public IService {
public:
    explicit TSubscriptionsService(
        const TServicesCommonContext& servicesCommonContext
    );

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    void OnSubscriptionsDevicesHttpRequest(const NNeh::IRequestRef& request);
    void OnSubscriptionsHttpRequest(const NNeh::IRequestRef& request);
    void OnSubscriptionsManageHttpRequest(const NNeh::IRequestRef& request);
    void OnSubscriptionsUserListHttpRequest(const NNeh::IRequestRef& request);

private:
    TIoTClient IoTClient_;
    TPushesAndNotificationsClient PushesAndNotificationsClient_;
    TTvmClient& TvmClient_;
    TRtLogClient& RtLogClient_;
    const TUserWhiteList UserWhiteList_;
};

} // namespace NMatrix::NNotificator
