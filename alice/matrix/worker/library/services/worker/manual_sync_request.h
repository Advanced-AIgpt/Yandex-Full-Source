#pragma once

#include "loop.h"

#include <alice/matrix/worker/library/services/worker/protos/service.pb.h>

#include <alice/matrix/library/request/typed_apphost_request.h>

namespace NMatrix::NWorker {

class TManualSyncRequest : public TTypedAppHostRequest<
    NServiceProtos::TManualSyncRequest,
    NServiceProtos::TManualSyncResponse,
    NEvClass::TMatrixWorkerManualSyncRequestData,
    NEvClass::TMatrixWorkerManualSyncResponseData
> {
public:
    TManualSyncRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        NAppHost::TTypedServiceContextPtr ctx,
        const NServiceProtos::TManualSyncRequest& request,
        TWorkerLoop& workerLoop
    );

    NThreading::TFuture<void> ServeAsync() override;

public:
    static inline constexpr TStringBuf NAME = "manual_sync";

private:
    const NPrivateApi::TManualSyncRequest& ApiRequest_;
    NPrivateApi::TManualSyncResponse& ApiResponse_;

    TWorkerLoop& WorkerLoop_;
};

} // namespace NMatrix::NWorker
