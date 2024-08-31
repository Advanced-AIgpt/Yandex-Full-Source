#pragma once

#include <alice/matrix/notificator/library/services/common_context/common_context.h>
#include <alice/matrix/notificator/library/services/update_device_environment/protos/service.apphost.h>

#include <alice/matrix/library/services/iface/service.h>


namespace NMatrix::NNotificator {

class TUpdateDeviceEnvironmentService
    : public IService
    , public NServiceProtos::TUpdateDeviceEnvironmentServiceAsync
{
public:
    explicit TUpdateDeviceEnvironmentService(
        const TServicesCommonContext& servicesCommonContext
    );

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    NThreading::TFuture<NServiceProtos::TUpdateDeviceEnvironmentResponse> UpdateDeviceEnvironment(
        NAppHost::TTypedServiceContextPtr ctx,
        const NServiceProtos::TUpdateDeviceEnvironmentRequest* request
    ) override;

private:
    TRtLogClient& RtLogClient_;
};

} // namespace NMatrix::NNotificator
