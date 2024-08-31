#include "manual_sync_request.h"

namespace NMatrix::NWorker {

TManualSyncRequest::TManualSyncRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    NAppHost::TTypedServiceContextPtr ctx,
    const NServiceProtos::TManualSyncRequest& request,
    TWorkerLoop& workerLoop
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
    , WorkerLoop_(workerLoop)
{}

NThreading::TFuture<void> TManualSyncRequest::ServeAsync() {
    // WARNING: AppHost thread is blocked here (it is intended for private api)
    if (auto syncResult = WorkerLoop_.ManualSync(LogContext_); !syncResult) {
        SetError(syncResult.Error());
    }

    return NThreading::MakeFuture();
}

} // namespace NMatrix::NWorker
