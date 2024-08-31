#include "service.h"

#include "update_device_environment_request.h"

#include <alice/matrix/library/services/typed_apphost_service/utils.h>


namespace NMatrix::NNotificator {

TUpdateDeviceEnvironmentService::TUpdateDeviceEnvironmentService(
    const TServicesCommonContext& servicesCommonContext
)
    : RtLogClient_(servicesCommonContext.RtLogClient)
{}

NThreading::TFuture<NServiceProtos::TUpdateDeviceEnvironmentResponse> TUpdateDeviceEnvironmentService::UpdateDeviceEnvironment(
    NAppHost::TTypedServiceContextPtr ctx,
    const NServiceProtos::TUpdateDeviceEnvironmentRequest* request
) {
    if (IsSuspended()) {
        return CreateTypedAppHostServiceIsSuspendedFastError<NServiceProtos::TUpdateDeviceEnvironmentResponse>();
    }

    auto req = MakeIntrusive<TUpdateDeviceEnvironmentRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        ctx,
        *request
    );

    if (req->IsFinished()) {
        return req->Reply();
    }

    return req->ServeAsync().Apply([req](const NThreading::TFuture<void>& fut) {
        return req->ReplyWithFutureCheck(fut);
    });
}

bool TUpdateDeviceEnvironmentService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    loop.RegisterService(port, *this);

    // Init metrics
    TSourceMetrics metrics(TUpdateDeviceEnvironmentRequest::NAME);
    metrics.InitAppHostResponseOk();

    return true;
}

} // namespace NMatrix::NNotificator
