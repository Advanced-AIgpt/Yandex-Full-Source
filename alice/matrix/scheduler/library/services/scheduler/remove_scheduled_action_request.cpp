#include "remove_scheduled_action_request.h"

namespace NMatrix::NScheduler {

namespace {

TExpected<TVector<TString>, TString> GetScheduledActionsToRemove(
    const google::protobuf::RepeatedPtrField<NApi::TRemoveScheduledActionRequest> removeScheduledActionRequests,
    const ui64 maxScheduledActionsToRemoveInOneAppHostRequest
) {
    if (removeScheduledActionRequests.empty()) {
        static const TString error = "Remove scheduled action requests are not provided";
        return error;
    }

    TVector<TString> scheduledActionsToRemove;
    scheduledActionsToRemove.reserve(removeScheduledActionRequests.size());
    THashSet<TString> scheduledActionIds;

    if (static_cast<ui64>(removeScheduledActionRequests.size()) > maxScheduledActionsToRemoveInOneAppHostRequest) {
        return TString::Join(
            "Too many scheduled actions to remove in one apphost request"
            ", actual number of scheduled actions to remove is ", ToString(removeScheduledActionRequests.size()),
            ", max allowed number is ", ToString(maxScheduledActionsToRemoveInOneAppHostRequest)
        );
    }

    for (const auto& removeScheduledActionRequest : removeScheduledActionRequests) {
        if (!scheduledActionIds.insert(removeScheduledActionRequest.GetActionId()).second) {
            return TString::Join("Two or more scheduled actions with id '", removeScheduledActionRequest.GetActionId(), '\'');
        }

        if (removeScheduledActionRequest.GetActionId().empty()) {
            static const TString error = "Action id must be non-empty";
            return error;
        }

        scheduledActionsToRemove.emplace_back(removeScheduledActionRequest.GetActionId());
    }

    return scheduledActionsToRemove;

}

} // namespace

TRemoveScheduledActionRequest::TRemoveScheduledActionRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    NAppHost::TTypedServiceContextPtr ctx,
    const NServiceProtos::TRemoveScheduledActionRequest& request,
    TSchedulerStorage& schedulerStorage,
    const ui64 maxScheduledActionsToRemoveInOneAppHostRequest
)
    : TTypedAppHostRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ false,
        rtLogClient,
        ctx,
        request
    )
    , ScheduledActionsToRemove_(
        GetScheduledActionsToRemove(
            Request_.GetApiRequests(),
            maxScheduledActionsToRemoveInOneAppHostRequest
        )
    )
    , SchedulerStorage_(schedulerStorage)
{
    if (IsFinished()) {
        return;
    }

    if (ScheduledActionsToRemove_.IsError()) {
        LogContext_.LogEventInfoCombo<NEvClass::TMatrixSchedulerRemoveScheduledActionsValidationError>(ScheduledActionsToRemove_.Error());

        SetError(TString::Join("Failed to build scheduled actions to remove: ", ScheduledActionsToRemove_.Error()));
        IsFinished_ = true;
        return;
    }
}

NThreading::TFuture<void> TRemoveScheduledActionRequest::ServeAsync() {
    const auto& scheduledActionsToRemove = ScheduledActionsToRemove_.Success();

    for (const auto& scheduledActionToRemove : scheduledActionsToRemove) {
        LogContext_.LogEventInfoCombo<NEvClass::TMatrixSchedulerRemoveScheduledAction>(scheduledActionToRemove);
    }

    return SchedulerStorage_.RemoveScheduledActions(
        scheduledActionsToRemove,
        LogContext_,
        Metrics_
    ).Apply(
        [this](const NThreading::TFuture<TExpected<void, TString>>& fut) {
            if (const auto& res = fut.GetValueSync(); !res) {
                SetError(res.Error());
                return;
            }

            for (ssize_t i = 0; i < Request_.GetApiRequests().size(); ++i) {
                Response_.AddApiResponses();
            }
        }
   );
}

} // namespace NMatrix::NScheduler
