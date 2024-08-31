#include "service.h"

#include "manual_sync_request.h"

#include <alice/matrix/library/services/typed_apphost_service/utils.h>

namespace NMatrix::NWorker {

TWorkerService::TWorkerService(
    const TServicesCommonContext& servicesCommonContext
)
    : WorkerLoop_(
        servicesCommonContext.Config.GetWorkerService().GetWorkerLoop(),
        servicesCommonContext.YDBDriver,
        servicesCommonContext.RtLogClient
    )
    , RtLogClient_(servicesCommonContext.RtLogClient)
{}

NThreading::TFuture<NServiceProtos::TManualSyncResponse> TWorkerService::ManualSync(
    NAppHost::TTypedServiceContextPtr ctx,
    const NServiceProtos::TManualSyncRequest* request
) {
    if (IsSuspended()) {
        return CreateTypedAppHostServiceIsSuspendedFastError<NServiceProtos::TManualSyncResponse>();
    }

    auto req = MakeIntrusive<TManualSyncRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        ctx,
        *request,
        WorkerLoop_
    );

    if (req->IsFinished()) {
        return req->Reply();
    }

    return req->ServeAsync().Apply([req](const NThreading::TFuture<void>& fut) {
        return req->ReplyWithFutureCheck(fut);
    });
}

bool TWorkerService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    loop.RegisterService(port, *this);

    // Init metrics
    TSourceMetrics metrics(TManualSyncRequest::NAME);
    metrics.InitAppHostResponseOk();

    return true;
}


void TWorkerService::SyncShutdown() {
    // It is necessary to avoid "CLIENT_CANCELLED\n<main>: Error: Client is stopped" error
    //
    // Without this explicit TWorkerLoop::SyncShutdown the following sequence of actions is possible, which leads to an error:
    //     1) TWorkerService::SyncShutdown
    //     2) YDBDriver.Stop
    //     3) Some YDB operation in TWorkerLoop
    //     4) TWorkerService::~TWorkerService
    //     5) TWorkerLoop::~TWorkerLoop
    //     6) TWorkerLoop::SyncShutdown
    WorkerLoop_.SyncShutdown();

    IService::SyncShutdown();
}

} // namespace NMatrix::NWorker
