#include "unschedule_http_request.h"

namespace NMatrix::NScheduler {

TUnscheduleHttpRequest::TUnscheduleHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TSchedulerStorage& schedulerStorage
)
    : TProtoHttpRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ false,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            return NNeh::NHttp::ERequestType::Post == method;
        }
    )
    , SchedulerStorage_(schedulerStorage)
{
    if (IsFinished()) {
        return;
    }

    if (Request_->GetActionId().empty()) {
        SetError("Action id must be non-empty", 400);
        IsFinished_ = true;
        return;
    }
}

NThreading::TFuture<void> TUnscheduleHttpRequest::ServeAsync() {
    LogContext_.LogEventInfoCombo<NEvClass::TMatrixSchedulerRemoveScheduledAction>(
        Request_->GetActionId()
    );

    return SchedulerStorage_.RemoveScheduledActions(
        {Request_->GetActionId()},
        LogContext_,
        Metrics_
    ).Apply(
        [this](const NThreading::TFuture<TExpected<void, TString>>& fut) {
            if (const auto& res = fut.GetValueSync(); !res) {
                SetError(res.Error(), 500);
            }
        }
    );
}

TUnscheduleHttpRequest::TReply TUnscheduleHttpRequest::GetReply() const {
    return TReply(200);
}

} // namespace NMatrix::NScheduler
