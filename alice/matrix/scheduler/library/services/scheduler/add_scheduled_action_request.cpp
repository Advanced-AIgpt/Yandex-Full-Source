#include "add_scheduled_action_request.h"

#include "utils.h"

#include <util/random/random.h>


namespace NMatrix::NScheduler {

namespace {

TExpected<TVector<TSchedulerStorage::TScheduledActionToAdd>, TString> GetScheduledActionsToAdd(
    const google::protobuf::RepeatedPtrField<NApi::TAddScheduledActionRequest> addScheduledActionRequests,
    const ui64 shardCount,
    const ui64 maxScheduledActionsToAddInOneAppHostRequest,
    TLogContext logContext
) {
    if (addScheduledActionRequests.empty()) {
        static const TString error = "Add scheduled action requests are not provided";
        return error;
    }

    TVector<TSchedulerStorage::TScheduledActionToAdd> scheduledActionsToAdd;
    scheduledActionsToAdd.reserve(addScheduledActionRequests.size());
    THashSet<TString> scheduledActionIds;

    if (static_cast<ui64>(addScheduledActionRequests.size()) > maxScheduledActionsToAddInOneAppHostRequest) {
        return TString::Join(
            "Too many scheduled actions to add in one apphost request"
            ", actual number of scheduled actions to add is ", ToString(addScheduledActionRequests.size()),
            ", max allowed number is ", ToString(maxScheduledActionsToAddInOneAppHostRequest)
        );
    }

    for (const auto& addScheduledActionRequest : addScheduledActionRequests) {
        if (!scheduledActionIds.insert(addScheduledActionRequest.GetMeta().GetId()).second) {
            return TString::Join("Two or more scheduled actions with id '", addScheduledActionRequest.GetMeta().GetId(), '\'');
        }

        const auto scheduledAction = CreateScheduledActionFromMetaAndSpec(
            addScheduledActionRequest.GetMeta(),
            addScheduledActionRequest.GetSpec()
        );

        if (!scheduledAction) {
            logContext.LogEventInfoCombo<NEvClass::TMatrixSchedulerAddScheduledActionValidationError>(
                addScheduledActionRequest.GetMeta().GetId(),
                scheduledAction.Error()
            );
            return TString::Join("Failed to build scheduled action '", addScheduledActionRequest.GetMeta().GetId(), "': ", scheduledAction.Error());
        }

        scheduledActionsToAdd.emplace_back(TSchedulerStorage::TScheduledActionToAdd({
            .ShardId = RandomNumber<ui64>(shardCount),
            .OverrideMode = addScheduledActionRequest.GetOverrideMode(),
            .ScheduledAction = scheduledAction.Success(),
        }));
    }

    return scheduledActionsToAdd;
}

} // namespace

TAddScheduledActionRequest::TAddScheduledActionRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    NAppHost::TTypedServiceContextPtr ctx,
    const NServiceProtos::TAddScheduledActionRequest& request,
    TSchedulerStorage& schedulerStorage,
    const ui64 shardCount,
    const ui64 maxScheduledActionsToAddInOneAppHostRequest
)
    : TTypedAppHostRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ false,
        rtLogClient,
        ctx,
        request
    )
    , ScheduledActionsToAdd_(
        GetScheduledActionsToAdd(
            Request_.GetApiRequests(),
            shardCount,
            maxScheduledActionsToAddInOneAppHostRequest,
            LogContext_
        )
    )
    , SchedulerStorage_(schedulerStorage)
{
    if (IsFinished()) {
        return;
    }

    if (ScheduledActionsToAdd_.IsError()) {
        LogContext_.LogEventInfoCombo<NEvClass::TMatrixSchedulerAddScheduledActionsValidationError>(ScheduledActionsToAdd_.Error());

        SetError(TString::Join("Failed to build scheduled actions to add: ", ScheduledActionsToAdd_.Error()));
        IsFinished_ = true;
        return;
    }
}

NThreading::TFuture<void> TAddScheduledActionRequest::ServeAsync() {
    const auto& scheduledActionsToAdd = ScheduledActionsToAdd_.Success();

    for (const auto& scheduledActionToAdd : scheduledActionsToAdd) {
        LogContext_.LogEventInfoCombo<NEvClass::TMatrixSchedulerAddScheduledAction>(
            scheduledActionToAdd.ScheduledAction.GetMeta().GetId(),
            scheduledActionToAdd.ScheduledAction.GetMeta().GetGuid(),
            scheduledActionToAdd.ScheduledAction,
            scheduledActionToAdd.ShardId,
            scheduledActionToAdd.OverrideMode
        );
    }

    return SchedulerStorage_.AddScheduledActions(
        scheduledActionsToAdd,
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
