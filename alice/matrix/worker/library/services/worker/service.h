#pragma once

#include "loop.h"

#include <alice/matrix/worker/library/services/common_context/common_context.h>
#include <alice/matrix/worker/library/services/worker/protos/service.apphost.h>

#include <alice/matrix/library/services/iface/service.h>


namespace NMatrix::NWorker {

class TWorkerService
    : public IService
    , public NServiceProtos::TWorkerServiceAsync
{
public:
    explicit TWorkerService(
        const TServicesCommonContext& servicesCommonContext
    );

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;
    void SyncShutdown() override;

private:
    NThreading::TFuture<NServiceProtos::TManualSyncResponse> ManualSync(
        NAppHost::TTypedServiceContextPtr ctx,
        const NServiceProtos::TManualSyncRequest* request
    ) override;

private:
    TWorkerLoop WorkerLoop_;
    TRtLogClient& RtLogClient_;
};

} // namespace NMatrix::NWorker
