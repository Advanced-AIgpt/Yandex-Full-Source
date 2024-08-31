#include "update_device_environment_request.h"

namespace NMatrix::NNotificator {

TUpdateDeviceEnvironmentRequest::TUpdateDeviceEnvironmentRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    NAppHost::TTypedServiceContextPtr ctx,
    const NServiceProtos::TUpdateDeviceEnvironmentRequest& request
)
    : TTypedAppHostRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ false,
        rtLogClient,
        ctx,
        request
    )
    , ApiRequest_(Request_.GetApiRequest())
    , ApiResponse_(*Response_.MutableApiResponse())
{
    if (IsFinished()) {
        return;
    }

    if (!Request_.HasApiRequest()) {
        SetError("ApiRequest not found in request");
        Metrics_.SetError("api_request_not_found");
        IsFinished_ = true;
        return;
    }

    if (ApiRequest_.GetType() == NApi::EDeviceEnvironmentType::NOT_SET) {
        SetError("Device environment type is not set");
        Metrics_.SetError("device_environment_not_set");
        IsFinished_ = true;
    }
}

NThreading::TFuture<void> TUpdateDeviceEnvironmentRequest::ServeAsync() {
    return NThreading::MakeFuture();
}

} // namespace NMatrix::NNotificator
