#include "service.h"

#include "add_scheduled_action_request.h"
#include "remove_scheduled_action_request.h"
#include "schedule_http_request.h"
#include "unschedule_http_request.h"

#include <alice/matrix/library/services/typed_apphost_service/utils.h>

namespace NMatrix::NScheduler {

TSchedulerService::TSchedulerService(
    const TServicesCommonContext& servicesCommonContext
)
    : SchedulerStorage_(
        servicesCommonContext.YDBDriver,
        servicesCommonContext.Config.GetSchedulerService().GetYDBClient()
    )
    , RtLogClient_(servicesCommonContext.RtLogClient)
    , ShardCount_(servicesCommonContext.Config.GetSchedulerService().GetShardCount())
    , MaxScheduledActionsToAddInOneAppHostRequest_(servicesCommonContext.Config.GetSchedulerService().GetMaxScheduledActionsToAddInOneAppHostRequest())
    , MaxScheduledActionsToRemoveInOneAppHostRequest_(servicesCommonContext.Config.GetSchedulerService().GetMaxScheduledActionsToRemoveInOneAppHostRequest())
{}

NThreading::TFuture<NServiceProtos::TAddScheduledActionResponse> TSchedulerService::AddScheduledAction(
    NAppHost::TTypedServiceContextPtr ctx,
    const NServiceProtos::TAddScheduledActionRequest* request
) {
    if (IsSuspended()) {
        return CreateTypedAppHostServiceIsSuspendedFastError<NServiceProtos::TAddScheduledActionResponse>();
    }

    auto req = MakeIntrusive<TAddScheduledActionRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        ctx,
        *request,
        SchedulerStorage_,
        ShardCount_,
        MaxScheduledActionsToAddInOneAppHostRequest_
    );

    if (req->IsFinished()) {
        return req->Reply();
    }

    return req->ServeAsync().Apply([req](const NThreading::TFuture<void>& fut) {
        return req->ReplyWithFutureCheck(fut);
    });
}

NThreading::TFuture<NServiceProtos::TRemoveScheduledActionResponse> TSchedulerService::RemoveScheduledAction(
    NAppHost::TTypedServiceContextPtr ctx,
    const NServiceProtos::TRemoveScheduledActionRequest* request
) {
    if (IsSuspended()) {
        return CreateTypedAppHostServiceIsSuspendedFastError<NServiceProtos::TRemoveScheduledActionResponse>();
    }

    auto req = MakeIntrusive<TRemoveScheduledActionRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        ctx,
        *request,
        SchedulerStorage_,
        MaxScheduledActionsToRemoveInOneAppHostRequest_
    );

    if (req->IsFinished()) {
        return req->Reply();
    }

    return req->ServeAsync().Apply([req](const NThreading::TFuture<void>& fut) {
        return req->ReplyWithFutureCheck(fut);
    });
}

void TSchedulerService::OnScheduleHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TScheduleHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request,
        SchedulerStorage_,
        ShardCount_
    );

    if (req->IsFinished()) {
        req->Reply();
        return;
    }

    if (IsSuspended()) {
        req->SetError("Service is suspended", 503);
        req->Reply();
        return;
    }

    req->ServeAsync().Apply([req](const NThreading::TFuture<void>& fut) {
        return req->ReplyWithFutureCheck(fut);
    });
}

void TSchedulerService::OnUnscheduleHttpRequest(const NNeh::IRequestRef& request) {
    auto req = MakeIntrusive<TUnscheduleHttpRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        request,
        SchedulerStorage_
    );

    if (req->IsFinished()) {
        req->Reply();
        return;
    }

    if (IsSuspended()) {
        req->SetError("Service is suspended", 503);
        req->Reply();
        return;
    }

    req->ServeAsync().Apply([req](const NThreading::TFuture<void>& fut) {
        return req->ReplyWithFutureCheck(fut);
    });
}

bool TSchedulerService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    loop.RegisterService(port, *this);

    {
        // Init metrics
        TSourceMetrics metrics(TAddScheduledActionRequest::NAME);
        metrics.InitAppHostResponseOk();
    }

    {
        // Init metrics
        TSourceMetrics metrics(TRemoveScheduledActionRequest::NAME);
        metrics.InitAppHostResponseOk();
    }

    {
        loop.Add(port, TString(TScheduleHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
            OnScheduleHttpRequest(request);
        });

        // Init metrics
        TSourceMetrics metrics(TScheduleHttpRequest::NAME);
        metrics.InitHttpCode(200, TScheduleHttpRequest::PATH.substr(1));
    }

    {
        loop.Add(port, TString(TUnscheduleHttpRequest::PATH), [this](const NNeh::IRequestRef& request) {
            OnUnscheduleHttpRequest(request);
        });

        // Init metrics
        TSourceMetrics metrics(TUnscheduleHttpRequest::NAME);
        metrics.InitHttpCode(200, TUnscheduleHttpRequest::PATH.substr(1));
    }

    return true;
}

} // namespace NMatrix::NScheduler
