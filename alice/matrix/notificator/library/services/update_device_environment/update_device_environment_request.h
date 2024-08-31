#pragma once

#include <alice/matrix/notificator/library/services/update_device_environment/protos/service.pb.h>

#include <alice/matrix/library/request/typed_apphost_request.h>

namespace NMatrix::NNotificator {

class TUpdateDeviceEnvironmentRequest : public TTypedAppHostRequest<
    NServiceProtos::TUpdateDeviceEnvironmentRequest,
    NServiceProtos::TUpdateDeviceEnvironmentResponse,
    NEvClass::TMatrixNotificatorUpdateDeviceEnvironmentRequestData,
    NEvClass::TMatrixNotificatorUpdateDeviceEnvironmentResponseData
> {
public:
    TUpdateDeviceEnvironmentRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        NAppHost::TTypedServiceContextPtr ctx,
        const NServiceProtos::TUpdateDeviceEnvironmentRequest& request
    );

    NThreading::TFuture<void> ServeAsync() override;

public:
    static inline constexpr TStringBuf NAME = "update_device_environment";

private:
    const NApi::TUpdateDeviceEnvironmentRequest& ApiRequest_;
    NApi::TUpdateDeviceEnvironmentResponse& ApiResponse_;
};

} // namespace NMatrix::NNotificator
