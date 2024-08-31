#pragma once

#include <alice/matrix/scheduler/library/services/common_context/common_context.h>
#include <alice/matrix/scheduler/library/services/scheduler/protos/service.apphost.h>
#include <alice/matrix/scheduler/library/storages/scheduler/storage.h>

#include <alice/matrix/library/services/iface/service.h>

namespace NMatrix::NScheduler {

class TSchedulerService
    : public IService
    , public NServiceProtos::TSchedulerServiceAsync
{
public:
    explicit TSchedulerService(
        const TServicesCommonContext& servicesCommonContext
    );

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    NThreading::TFuture<NServiceProtos::TAddScheduledActionResponse> AddScheduledAction(
        NAppHost::TTypedServiceContextPtr ctx,
        const NServiceProtos::TAddScheduledActionRequest* request
    ) override;

    NThreading::TFuture<NServiceProtos::TRemoveScheduledActionResponse> RemoveScheduledAction(
        NAppHost::TTypedServiceContextPtr ctx,
        const NServiceProtos::TRemoveScheduledActionRequest* request
    ) override;

    void OnScheduleHttpRequest(const NNeh::IRequestRef& request);
    void OnUnscheduleHttpRequest(const NNeh::IRequestRef& request);

private:
    TSchedulerStorage SchedulerStorage_;
    TRtLogClient& RtLogClient_;

    const ui64 ShardCount_;
    const ui64 MaxScheduledActionsToAddInOneAppHostRequest_;
    const ui64 MaxScheduledActionsToRemoveInOneAppHostRequest_;
};

} // namespace NMatrix::NScheduler
