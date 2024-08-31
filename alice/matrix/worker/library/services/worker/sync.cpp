#include "sync.h"
#include "utils.h"

#include <alice/protos/api/matrix/delivery.pb.h>
#include <alice/protos/api/matrix/technical_push.pb.h>
#include <alice/protos/api/matrix/user_device.pb.h>

#include <library/cpp/protobuf/interop/cast.h>
#include <library/cpp/watchdog/watchdog.h>

#include <google/protobuf/any.pb.h>
#include <google/protobuf/timestamp.pb.h>

#include <util/generic/guid.h>

namespace NMatrix::NWorker {

namespace {

using TNotificatorResponseRef = std::variant<std::reference_wrapper<const NMatrix::NApi::TDeliveryResponse>, std::reference_wrapper<const TString>>;

NMonitoring::IHistogramCollectorPtr GetDurationHistogramCollector() {
    return NMonitoring::ExponentialHistogram(
        30,
        2,
        TDuration::MilliSeconds(1).MicroSeconds()
    );
}

} // namespace

std::once_flag TWorkerSync::INIT_METRICS_ONCE_FLAG;

TWorkerSync::TWorkerSync(
    TWorkerStorage& workerStorage,
    TRtLogClient& rtLogClient,
    TKeepAliveHttpClient& matrixNotificatorClient,
    const TString& syncGuid,
    ui64 selectLimit,
    TDuration perActionTimeout,
    TDuration maxHeartbeatInactivityPeriodToAcquireLock,
    TDuration maxHeartbeatInactivityPeriodToReleaseLock,
    TDuration maxHeartbeatInactivityPeriodToEnsureLeading,
    TDuration minEnsureShardLockLeadingAndDoHeartbeatPeriod
)
    : WorkerStorage_(workerStorage)
    , RtLogClient_(rtLogClient)
    , MatrixNotificatorClient_(matrixNotificatorClient)

    , StartTime_(TInstant::Now())
    , Metrics_(NAME)

    , SyncGuid_(syncGuid)
    , SelectLimit_(selectLimit)
    , PerActionTimeout_(perActionTimeout)
    , MaxHeartbeatInactivityPeriodToAcquireLock_(maxHeartbeatInactivityPeriodToAcquireLock)
    , MaxHeartbeatInactivityPeriodToReleaseLock_(maxHeartbeatInactivityPeriodToReleaseLock)
    , MaxHeartbeatInactivityPeriodToEnsureLeading_(maxHeartbeatInactivityPeriodToEnsureLeading)
    , MinEnsureShardLockLeadingAndDoHeartbeatPeriod_(minEnsureShardLockLeadingAndDoHeartbeatPeriod)

    , EnsureShardLockLeadingFailedWithError_(Nothing())
    , LastEnsureShardLockLeadingAt_(TInstant::Zero())
{
    std::call_once(
        INIT_METRICS_ONCE_FLAG,
        std::bind(&TWorkerSync::InitMetrics, this)
    );
}

TWorkerSync::~TWorkerSync() {
    try {
        Metrics_.PushTimeDiffWithNowHist(
            StartTime_,
            "sync_time",
            "",
            "self",
            {},
            GetDurationHistogramCollector()
        );
    } catch (...) {
    }
}

TExpected<TWorkerSync::ESyncStatus, TString> TWorkerSync::Run(
    TLogContext logContext
) {
    TWorkerSync::ESyncStatus status = ESyncStatus::PERFORMED;
    TMaybe<TString> error;

    logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerSyncStarted>(SyncGuid_);

    // AcquireShardLock == EnsureShardLockLeading
    LastEnsureShardLockLeadingAt_ = TInstant::Now();
    if (auto res = WorkerStorage_.AcquireShardLock(SyncGuid_, MaxHeartbeatInactivityPeriodToAcquireLock_, logContext, Metrics_).GetValueSync(); res) {
        auto acquireShardLockResult = res.Success();

        switch (acquireShardLockResult.Status) {
            case TWorkerStorage::TAcquireShardLockResult::EStatus::OK: {
                const auto& shardLockInfo = *acquireShardLockResult.ShardLockInfo;
                Metrics_.PushRate("acquire_shard_lock", "ok");

                logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerShardLockAcquireSuccess>(
                    shardLockInfo.ShardId,
                    shardLockInfo.Locked,
                    shardLockInfo.LastLockGuid,
                    shardLockInfo.LastLockedBy,
                    ToString(shardLockInfo.LastProcessingStartAt),
                    ToString(shardLockInfo.LastHeartbeatAt)
                );

                if (shardLockInfo.Locked) {
                    Metrics_.PushRate("interrupted_sync_found", "error");
                    logContext.LogEventErrorCombo<NEvClass::TMatrixWorkerInterruptedSyncFound>(
                        shardLockInfo.ShardId,
                        shardLockInfo.LastLockGuid,
                        shardLockInfo.LastLockedBy,
                        SyncGuid_
                    );
                }

                // Useless check just for symmetry
                if (!EnsureShardLockLeadingFailedWithError_.Defined()) {
                    if (auto res = MoveActionRowsFromIncomingToProcessing(shardLockInfo.ShardId, logContext); res) {
                        Metrics_.PushRate("move_action_rows_from_incoming_to_processing", "ok");
                        logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerMoveActionRowsFromIncomingToProcessingSuccess>(
                            SyncGuid_,
                            shardLockInfo.ShardId
                        );
                    } else {
                        Metrics_.PushRate("move_action_rows_from_incoming_to_processing", "error");
                        logContext.LogEventErrorCombo<NEvClass::TMatrixWorkerMoveActionRowsFromIncomingToProcessingError>(
                            SyncGuid_,
                            shardLockInfo.ShardId,
                            res.Error()
                        );
                    }
                }

                if (!EnsureShardLockLeadingFailedWithError_.Defined()) {
                    if (auto res = PerformActionsFromProcessing(shardLockInfo.ShardId, logContext); res) {
                        Metrics_.PushRate("perform_actions_from_processing", "ok");
                        logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerPerformActionsFromProcessingSuccess>(
                            SyncGuid_,
                            shardLockInfo.ShardId
                        );
                    } else {
                        Metrics_.PushRate("perform_actions_from_processing", "error");
                        logContext.LogEventErrorCombo<NEvClass::TMatrixWorkerPerformActionsFromProcessingError>(
                            SyncGuid_,
                            shardLockInfo.ShardId,
                            res.Error()
                        );
                    }
                }

                if (!EnsureShardLockLeadingFailedWithError_.Defined()) {
                    if (auto res = WorkerStorage_.ReleaseShardLock(shardLockInfo.ShardId, SyncGuid_, MaxHeartbeatInactivityPeriodToReleaseLock_, logContext, Metrics_).GetValueSync(); res) {
                        Metrics_.PushRate("release_shard_lock", "ok");
                    } else {
                        error = res.Error();

                        Metrics_.PushRate("release_shard_lock", "error");
                        Metrics_.SetError("release_shard_lock_error");
                        logContext.LogEventErrorCombo<NEvClass::TMatrixWorkerReleaseShardLockError>(
                            shardLockInfo.ShardId,
                            SyncGuid_,
                            *error
                        );
                    }
                }

                if (!error.Defined() && EnsureShardLockLeadingFailedWithError_.Defined()) {
                    error = EnsureShardLockLeadingFailedWithError_;
                }

                Metrics_.PushRate(
                    "sync_shard",
                    (error.Defined() ? "error" : "ok"),
                    "self",
                    {{"shard", ToString(shardLockInfo.ShardId)}}
                );

                break;
            }
            case TWorkerStorage::TAcquireShardLockResult::EStatus::NO_FREE_SHARD: {
                status = ESyncStatus::SKIPPED;
                Metrics_.PushRate("acquire_shard_lock", "no_free_shard");
                logContext.LogEventErrorCombo<NEvClass::TMatrixWorkerNoFreeShard>(SyncGuid_);
                break;
            }
            case TWorkerStorage::TAcquireShardLockResult::EStatus::TRANSACTION_LOCKS_INVALIDATED: {
                status = ESyncStatus::SKIPPED;
                Metrics_.PushRate("acquire_shard_lock", "transaction_locks_invalidated");
                logContext.LogEventErrorCombo<NEvClass::TMatrixWorkerAcquireShardLockTransactionLocksInvalidated>(SyncGuid_);
                break;
            }
        }
    } else {
        error = res.Error();

        Metrics_.PushRate("acquire_shard_lock", "error");
        Metrics_.SetError("acquire_shard_lock_error");
        logContext.LogEventErrorCombo<NEvClass::TMatrixWorkerShardLockAcquireError>(*error);
    }

    logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerSyncFinished>(
        SyncGuid_,
        ToString(TInstant::Now() - StartTime_)
    );

    if (error.Defined()) {
        Metrics_.PushRate("sync", "error");
        return *error;
    } else {
        switch (status) {
            case TWorkerSync::ESyncStatus::PERFORMED: {
                Metrics_.PushRate("sync", "ok");
                break;
            }
            case TWorkerSync::ESyncStatus::SKIPPED: {
                Metrics_.PushRate("sync", "skip");
                break;
            }
        }

        return status;
    }
}

void TWorkerSync::InitMetrics() {
    // Init special metrics to avoid no data in alerts

    // Sync
    Metrics_.PushRate(0, "sync", "ok");
    Metrics_.PushRate(0, "sync", "skip");
    Metrics_.PushRate(0, "sync", "error");
    Metrics_.PushRate(0, "interrupted_sync_found", "error");

    // Database operations with actions
    Metrics_.PushRate(0, "move_action_rows_from_incoming_to_processing", "ok");
    Metrics_.PushRate(0, "move_action_rows_from_incoming_to_processing", "error");
    Metrics_.PushRate(0, "move_action_row_from_incoming_to_processing", "ok");
    Metrics_.PushRate(0, "move_action_row_from_incoming_to_processing", "error");

    Metrics_.PushRate(0, "perform_actions_from_processing", "ok");
    Metrics_.PushRate(0, "perform_actions_from_processing", "error");
    Metrics_.PushRate(0, "perform_action_from_processing", "ok");
    Metrics_.PushRate(0, "perform_action_from_processing", "do_action_error");
    Metrics_.PushRate(0, "perform_action_from_processing", "error");

    Metrics_.InitHist(
        "perform_action_delay",
        GetDurationHistogramCollector(),
        "",
        "self"
    );

    // Do actions
    Metrics_.PushRate(0, "do_action", "ok");
    Metrics_.PushRate(0, "do_action", "error");

    Metrics_.PushRate(0, "do_mock_action", "ok");
    Metrics_.PushRate(0, "do_mock_action", "error");

    Metrics_.PushRate(0, "do_send_technical_push", "ok");
    Metrics_.PushRate(0, "do_send_technical_push", "error");
    Metrics_.PushRate(0, "do_send_technical_push", "device_not_connected");
}

TExpected<void, TString> TWorkerSync::EnsureShardLockLeading(
    ui64 shardId,
    TLogContext logContext
) {
    logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerEnsureShardLockLeadingStart>(
        SyncGuid_,
        shardId
    );

    TRtLogActivation rtLogActivation =
        logContext.RtLogPtr()
        ? TRtLogActivation(
            logContext.RtLogPtr(),
            "ensure-shard-lock-leading",
            /* newRequest = */ false
        )
        : TRtLogActivation()
    ;
    logContext = TLogContext(
        logContext.FramePtr(),
        RtLogClient_.CreateRequestLogger(rtLogActivation.Token())
    );

    if (EnsureShardLockLeadingFailedWithError_.Defined()) {
        TString error = TString::Join(
            "Shard lock leading already lost with error: ",
            *EnsureShardLockLeadingFailedWithError_
        );

        logContext.LogEventErrorCombo<NEvClass::TMatrixWorkerEnsureShardLockLeadingAlreadyFailed>(
            SyncGuid_,
            shardId,
            *EnsureShardLockLeadingFailedWithError_
        );
        rtLogActivation.Finish(/* ok = */ false, error);
        return error;
    }

    if (auto now = TInstant::Now(); now - LastEnsureShardLockLeadingAt_ < MinEnsureShardLockLeadingAndDoHeartbeatPeriod_) {
        logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerEnsureShardLockLeadingSkip>(
            SyncGuid_,
            shardId,
            ToString(now),
            ToString(LastEnsureShardLockLeadingAt_),
            ToString(MinEnsureShardLockLeadingAndDoHeartbeatPeriod_)
        );
        rtLogActivation.Finish(/* ok = */ true);

        return TExpected<void, TString>::DefaultSuccess();
    } else {
        LastEnsureShardLockLeadingAt_ = now;
        auto res = WorkerStorage_.EnsureShardLockLeadingAndDoHeartbeat(
            shardId,
            SyncGuid_,
            MaxHeartbeatInactivityPeriodToEnsureLeading_,
            logContext,
            Metrics_
        ).GetValueSync();

        // All logs in storage
        if (res) {
            rtLogActivation.Finish(/* ok = */ true);
            return TExpected<void, TString>::DefaultSuccess();
        } else {
            TString error = res.Error();

            EnsureShardLockLeadingFailedWithError_ = error;
            rtLogActivation.Finish(/* ok = */ false, error);
            return error;
        }
    }
}

TExpected<void, TString> TWorkerSync::MoveActionRowsFromIncomingToProcessing(
    ui64 shardId,
    TLogContext logContext
) {
    logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerMoveActionRowsFromIncomingToProcessingStart>(
        SyncGuid_,
        shardId,
        SelectLimit_
    );

    TRtLogActivation rtLogActivation =
        logContext.RtLogPtr()
        ? TRtLogActivation(
            logContext.RtLogPtr(),
            "move-action-rows-from-incoming-to-processing",
            /* newRequest = */ false
        )
        : TRtLogActivation()
    ;
    logContext = TLogContext(
        logContext.FramePtr(),
        RtLogClient_.CreateRequestLogger(rtLogActivation.Token())
    );

    if (auto res = EnsureShardLockLeading(shardId, logContext); !res) {
        rtLogActivation.Finish(/* ok = */ false, res.Error());
        return res.Error();
    }

    auto getIncomingActionRowsRes = WorkerStorage_.GetIncomingActionRows(
        shardId,
        SelectLimit_,
        logContext,
        Metrics_
    ).GetValueSync();
    if (!getIncomingActionRowsRes) {
        rtLogActivation.Finish(/* ok = */ false, getIncomingActionRowsRes.Error());
        return getIncomingActionRowsRes.Error();
    }
    const auto& incomingActionRows = getIncomingActionRowsRes.Success();

    Metrics_.PushRate(incomingActionRows.size(), "selected_incoming_action_rows");
    logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerIncomingActionRowsSelected>(incomingActionRows.size());

    for (auto& incomingActionRow : incomingActionRows) {
        if (auto res = EnsureShardLockLeading(shardId, logContext); !res) {
            rtLogActivation.Finish(/* ok = */ false, res.Error());
            return res.Error();
        }

        if (auto res = MoveActionRowFromIncomingToProcessing(incomingActionRow, logContext); res) {
            Metrics_.PushRate("move_action_row_from_incoming_to_processing", "ok");
            logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerMoveActionRowFromIncomingToProcessingSuccess>(
                incomingActionRow.ActionId,
                incomingActionRow.ActionGuid
            );
        } else {
            Metrics_.PushRate("move_action_row_from_incoming_to_processing", "error");
            logContext.LogEventErrorCombo<NEvClass::TMatrixWorkerMoveActionRowFromIncomingToProcessingError>(
                incomingActionRow.ActionId,
                incomingActionRow.ActionGuid,
                res.Error()
            );
        }
    }

    rtLogActivation.Finish(/* ok = */ true);
    return TExpected<void, TString>::DefaultSuccess();
}

TExpected<void, TString> TWorkerSync::MoveActionRowFromIncomingToProcessing(
    const TWorkerStorage::TIncomingActionRow& incomingActionRow,
    TLogContext logContext
) {
    logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerMoveActionRowFromIncomingToProcessingStart>(
        incomingActionRow.ShardId,
        ToString(incomingActionRow.CreatedAt),
        incomingActionRow.ActionId,
        incomingActionRow.ActionGuid,

        ToString(incomingActionRow.ScheduledAt)
    );

    TRtLogActivation rtLogActivation =
        logContext.RtLogPtr()
        ? TRtLogActivation(
            logContext.RtLogPtr(),
            TString::Join(
                "move-action-row-from-incoming-to-processing-",
                ToString(incomingActionRow.ShardId),  '-',
                incomingActionRow.ActionId, '-',
                incomingActionRow.ActionGuid
            ),
            /* newRequest = */ false
        )
        : TRtLogActivation()
    ;
    logContext = TLogContext(
        logContext.FramePtr(),
        RtLogClient_.CreateRequestLogger(rtLogActivation.Token())
    );

    auto res = WorkerStorage_.MoveActionRowFromIncomingToProcessing(
        incomingActionRow,
        SyncGuid_,
        logContext,
        Metrics_
    ).GetValueSync();

    if (res) {
        rtLogActivation.Finish(/* ok = */ true);
    } else {
        rtLogActivation.Finish(/* ok = */ false, res.Error());
    }

    return res;
}

TExpected<void, TString> TWorkerSync::PerformActionsFromProcessing(
    ui64 shardId,
    TLogContext logContext
) {
    logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerPerformActionsFromProcessingStart>(
        SyncGuid_,
        shardId,
        SelectLimit_
    );

    TRtLogActivation rtLogActivation =
        logContext.RtLogPtr()
        ? TRtLogActivation(
            logContext.RtLogPtr(),
            "perform-actions-from-processing",
            /* newRequest = */ false
        )
        : TRtLogActivation()
    ;
    logContext = TLogContext(
        logContext.FramePtr(),
        RtLogClient_.CreateRequestLogger(rtLogActivation.Token())
    );

    if (auto res = EnsureShardLockLeading(shardId, logContext); !res) {
        rtLogActivation.Finish(/* ok = */ false, res.Error());
        return res.Error();
    }

    auto getProcessingActionRowsRes = WorkerStorage_.GetProcessingActionRows(
        shardId,
        SelectLimit_,
        logContext,
        Metrics_
    ).GetValueSync();
    if (!getProcessingActionRowsRes) {
        rtLogActivation.Finish(/* ok = */ false, getProcessingActionRowsRes.Error());
        return getProcessingActionRowsRes.Error();
    }
    const auto& processingActionRows = getProcessingActionRowsRes.Success();

    Metrics_.PushRate(processingActionRows.size(), "selected_processing_action_rows");
    logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerProcessingActionRowsSelected>(processingActionRows.size());

    for (auto& processingActionRow : processingActionRows) {
        if (auto res = EnsureShardLockLeading(shardId, logContext); !res) {
            rtLogActivation.Finish(/* ok = */ false, res.Error());
            return res.Error();
        }

        if (auto res = PerformActionFromProcessing(processingActionRow, logContext); res) {
            switch (res.Success()) {
                case EPerfromActionStatus::SUCCESS: {
                    Metrics_.PushRate("perform_action_from_processing", "ok");
                    logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerPerformActionFromProcessingSuccess>(
                        processingActionRow.ActionId,
                        processingActionRow.ActionGuid
                    );
                    break;
                }
                case EPerfromActionStatus::DO_ACTION_ERROR: {
                    Metrics_.PushRate("perform_action_from_processing", "do_action_error");
                    logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerPerformActionFromProcessingDoActionError>(
                        processingActionRow.ActionId,
                        processingActionRow.ActionGuid
                    );
                    break;
                }
            }
        } else {
            Metrics_.PushRate("perform_action_from_processing", "error");
            logContext.LogEventErrorCombo<NEvClass::TMatrixWorkerPerformActionFromProcessingError>(
                processingActionRow.ActionId,
                processingActionRow.ActionGuid,
                res.Error()
            );
        }
    }

    rtLogActivation.Finish(/* ok = */ true);
    return TExpected<void, TString>::DefaultSuccess();
}

TExpected<TWorkerSync::EPerfromActionStatus, TString> TWorkerSync::PerformActionFromProcessing(
    const TWorkerStorage::TProcessingActionRow& processingActionRow,
    TLogContext logContext
) {
    TString abortWatchDogQuery = TString::Join(
        SyncGuid_,
        '-', ToString(processingActionRow.ShardId),
        '-', processingActionRow.ActionId,
        '-', processingActionRow.ActionGuid
    );
    THolder<IWatchDog> abortWatchDog = THolder<IWatchDog>(
        CreateAbortByTimeoutWatchDog(PerActionTimeout_, abortWatchDogQuery)
    );

    logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerPerformActionFromProcessingStart>(
        processingActionRow.ShardId,
        ToString(processingActionRow.ScheduledAt),
        processingActionRow.ActionId,
        processingActionRow.ActionGuid,

        ToString(processingActionRow.AddedToIncomingQueueAt),

        processingActionRow.MovedFromIncomingToProcessingQueueBySyncWithGuid,
        ToString(processingActionRow.MovedFromIncomingToProcessingQueueAt),

        processingActionRow.LastRescheduleBySyncWithGuid,
        ToString(processingActionRow.LastRescheduleAt)
    );

    Metrics_.PushTimeDiffWithNowHist(
        processingActionRow.ScheduledAt,
        "perform_action_delay",
        "",
        "self",
        {},
        GetDurationHistogramCollector()
    );

    TRtLogActivation rtLogActivation =
        logContext.RtLogPtr()
        ? TRtLogActivation(
            logContext.RtLogPtr(),
            TString::Join(
                "perform-action-from-processing-",
                ToString(processingActionRow.ShardId), '-',
                processingActionRow.ActionId, '-',
                processingActionRow.ActionGuid
            ),
            /* newRequest = */ false
        )
        : TRtLogActivation()
    ;
    logContext = TLogContext(
        logContext.FramePtr(),
        RtLogClient_.CreateRequestLogger(rtLogActivation.Token())
    );

    // Get scheduled action
    auto getScheduledActionRes = GetScheduledActionAndRemoveProcessingActionRowIfNeeded(
        processingActionRow,
        logContext
    );
    if (!getScheduledActionRes) {
        rtLogActivation.Finish(/* ok = */ false, getScheduledActionRes.Error());
        return getScheduledActionRes.Error();
    }

    const auto& scheduledActionMaybe = getScheduledActionRes.Success();
    if (!scheduledActionMaybe.Defined()) {
        rtLogActivation.Finish(/* ok = */ true);
        return TExpected<EPerfromActionStatus, TString>::DefaultSuccess();
    }
    auto scheduledAction = *scheduledActionMaybe;

    logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerScheduledActionData>(
        scheduledAction.GetMeta().GetId(),
        scheduledAction.GetMeta().GetGuid(),
        scheduledAction
    );

    if (auto res = EnsureShardLockLeading(processingActionRow.ShardId, logContext); !res) {
        rtLogActivation.Finish(/* ok = */ false, res.Error());
        return res.Error();
    }

    // Don't process action with mismatched guid
    if (auto res = CheckGuidAndRemoveProcessingActionRowIfNeeded(scheduledAction, processingActionRow, logContext); res.IsError()) {
        rtLogActivation.Finish(/* ok = */ false, res.Error());
        return res.Error();
    } else if (!res.Success()) {
        rtLogActivation.Finish(/* ok = */ true);
        return TExpected<EPerfromActionStatus, TString>::DefaultSuccess();
    }

    if (auto res = CheckDeadlineAndRemoveScheduledActionIfNeeded(scheduledAction, processingActionRow.ShardId, processingActionRow.ScheduledAt, logContext); res.IsError()) {
        rtLogActivation.Finish(/* ok = */ false, res.Error());
        return res.Error();
    } else if (!res.Success()) {
        rtLogActivation.Finish(/* ok = */ true);
        return TExpected<EPerfromActionStatus, TString>::DefaultSuccess();
    }

    PrepareScheduledActionStatusForCurrentAttempt(
        scheduledAction,
        logContext
    );
    if (auto res = CheckStatusAndRemoveScheduledActionIfNeeded(scheduledAction, processingActionRow.ShardId, processingActionRow.ScheduledAt, logContext); res.IsError()) {
        rtLogActivation.Finish(/* ok = */ false, res.Error());
        return res.Error();
    } else if (!res.Success()) {
        rtLogActivation.Finish(/* ok = */ true);
        return TExpected<EPerfromActionStatus, TString>::DefaultSuccess();
    }

    if (
        auto res = UpdateScheduledActionStatusAndRescheduleProcessingActionRow(
            scheduledAction,
            processingActionRow.ShardId,
            processingActionRow.ScheduledAt,
            processingActionRow.AddedToIncomingQueueAt,
            processingActionRow.MovedFromIncomingToProcessingQueueBySyncWithGuid,
            processingActionRow.MovedFromIncomingToProcessingQueueAt,
            logContext
        );
        !res
    ) {
        rtLogActivation.Finish(/* ok = */ false, res.Error());
        return res.Error();
    }

    if (auto res = EnsureShardLockLeading(processingActionRow.ShardId, logContext); !res) {
        rtLogActivation.Finish(/* ok = */ false, res.Error());
        return res.Error();
    }

    auto doActionRes = DoAction(scheduledAction.GetMeta(), scheduledAction.GetStatus(), scheduledAction.GetSpec().GetAction(), logContext);
    if (doActionRes) {
        Metrics_.PushRate("do_action", "ok");
        logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerDoActionSuccess>(
            scheduledAction.GetMeta().GetId(),
            scheduledAction.GetMeta().GetGuid()
        );
    } else {
        Metrics_.PushRate("do_action", "error");
        logContext.LogEventErrorCombo<NEvClass::TMatrixWorkerDoActionError>(
            scheduledAction.GetMeta().GetId(),
            scheduledAction.GetMeta().GetGuid(),
            doActionRes.Error()
        );
    }

    if (auto res = EnsureShardLockLeading(processingActionRow.ShardId, logContext); !res) {
        rtLogActivation.Finish(/* ok = */ false, res.Error());
        return res.Error();
    }

    // UpdateScheduledActionStatusAfterDoAction can change ScheduledAt
    // So, we have to save it here
    TInstant processingActionRowScheduledAt = NProtoInterop::CastFromProto(scheduledAction.GetStatus().GetScheduledAt());
    UpdateScheduledActionStatusAfterDoAction(scheduledAction, doActionRes);
    if (auto res = CheckStatusAndRemoveScheduledActionIfNeeded(scheduledAction, processingActionRow.ShardId, processingActionRowScheduledAt, logContext); res.IsError()) {
        rtLogActivation.Finish(/* ok = */ false, res.Error());
        return res.Error();
    } else if (!res.Success()) {
        if (doActionRes) {
            rtLogActivation.Finish(/* ok = */ true);
            return EPerfromActionStatus::SUCCESS;
        } else {
            rtLogActivation.Finish(/* ok = */ false, doActionRes.Error());
            return EPerfromActionStatus::DO_ACTION_ERROR;
        }
    }

    if (
        auto res = UpdateScheduledActionStatusAndRescheduleProcessingActionRow(
            scheduledAction,
            processingActionRow.ShardId,
            processingActionRowScheduledAt,
            processingActionRow.AddedToIncomingQueueAt,
            processingActionRow.MovedFromIncomingToProcessingQueueBySyncWithGuid,
            processingActionRow.MovedFromIncomingToProcessingQueueAt,
            logContext
        );
        !res
    ) {
        rtLogActivation.Finish(/* ok = */ false, res.Error());
        return res.Error();
    }

    if (doActionRes) {
        rtLogActivation.Finish(/* ok = */ true);
        return EPerfromActionStatus::SUCCESS;
    } else {
        rtLogActivation.Finish(/* ok = */ false, doActionRes.Error());
        return EPerfromActionStatus::DO_ACTION_ERROR;
    }
}

TExpected<TMaybe<NMatrix::NApi::TScheduledAction>, TString> TWorkerSync::GetScheduledActionAndRemoveProcessingActionRowIfNeeded(
    const TWorkerStorage::TProcessingActionRow& processingActionRow,
    TLogContext logContext
) {
    // Get scheduled action
    auto getScheduledActionRes = WorkerStorage_.GetScheduledAction(
        processingActionRow.ActionId,
        logContext,
        Metrics_
    ).GetValueSync();
    if (!getScheduledActionRes) {
        return getScheduledActionRes.Error();
    }

    const auto& scheduledActionMaybe = getScheduledActionRes.Success();
    if (!scheduledActionMaybe.Defined()) {
        Metrics_.PushRate("scheduled_action_not_found_in_database");
        logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerScheduledActionNotFoundInDatabase>(
            processingActionRow.ActionId,
            processingActionRow.ActionGuid
        );

        // Ensure shard leading before remove
        if (auto res = EnsureShardLockLeading(processingActionRow.ShardId, logContext); !res) {
            return res.Error();
        }

        if (auto res = WorkerStorage_.RemoveProcessingActionRow(processingActionRow, logContext, Metrics_).GetValueSync(); !res) {
            return res.Error();
        }
    }

    return scheduledActionMaybe;
}

TExpected<bool, TString> TWorkerSync::CheckGuidAndRemoveProcessingActionRowIfNeeded(
    const NMatrix::NApi::TScheduledAction& scheduledAction,
    const TWorkerStorage::TProcessingActionRow& processingActionRow,
    TLogContext logContext
) {
    if (scheduledAction.GetMeta().GetGuid() != processingActionRow.ActionGuid) {
        Metrics_.PushRate("scheduled_action_guid_mismatched_with_processing_action_row_guid");
        logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerScheduledActionGuidMismatchedWithProcessingActionRowGuid>(
            scheduledAction.GetMeta().GetId(),
            scheduledAction.GetMeta().GetGuid(),
            processingActionRow.ActionGuid
        );

        if (auto res = WorkerStorage_.RemoveProcessingActionRow(processingActionRow, logContext, Metrics_).GetValueSync(); !res) {
            return res.Error();
        }

        return false;
    }

    return true;
}

TExpected<bool, TString> TWorkerSync::CheckDeadlineAndRemoveScheduledActionIfNeeded(
    const NMatrix::NApi::TScheduledAction& scheduledAction,
    ui64 processingActionRowShardId,
    TInstant processingActionRowScheduledAt,
    TLogContext logContext
) {
    if (scheduledAction.GetSpec().GetSendPolicy().HasDeadline()) {
        const auto now = TInstant::Now();
        const auto deadline = NProtoInterop::CastFromProto(scheduledAction.GetSpec().GetSendPolicy().GetDeadline());
        if (deadline < now) {
            Metrics_.PushRate("scheduled_action_deadline_arrived");
            logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerScheduledActionDeadlineArrived>(
                scheduledAction.GetMeta().GetId(),
                scheduledAction.GetMeta().GetGuid(),
                ToString(now),
                ToString(deadline)
            );

            if (auto res = WorkerStorage_.RemoveScheduledActionWithProcessingActionRow(scheduledAction, processingActionRowShardId, processingActionRowScheduledAt, logContext, Metrics_).GetValueSync(); !res) {
                return res.Error();
            }

            return false;
        }
    }

    return true;
}

TExpected<bool, TString> TWorkerSync::CheckStatusAndRemoveScheduledActionIfNeeded(
    const NMatrix::NApi::TScheduledAction& scheduledAction,
    ui64 processingActionRowShardId,
    TInstant processingActionRowScheduledAt,
    TLogContext logContext
) {
    auto removeScheduledAction = [&]() -> TExpected<void, TString> {
        if (auto res = WorkerStorage_.RemoveScheduledActionWithProcessingActionRow(scheduledAction, processingActionRowShardId, processingActionRowScheduledAt, logContext, Metrics_).GetValueSync(); !res) {
            return res.Error();
        }

        return TExpected<void, TString>::DefaultSuccess();
    };

    switch (scheduledAction.GetSpec().GetSendPolicy().GetPolicyCase()) {
        case NMatrix::NApi::TScheduledActionSpec::TSendPolicy::kSendOncePolicy: {
            if (scheduledAction.GetStatus().GetConsecutiveFailuresCounter() > scheduledAction.GetSpec().GetSendPolicy().GetSendOncePolicy().GetRetryPolicy().GetMaxRetries()) {
                Metrics_.PushRate("scheduled_action_max_attempts_reached");
                logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerScheduledActionMaxAttemptsReached>(
                    scheduledAction.GetMeta().GetId(),
                    scheduledAction.GetMeta().GetGuid(),
                    scheduledAction.GetSpec().GetSendPolicy().GetSendOncePolicy().GetRetryPolicy().GetMaxRetries(),
                    scheduledAction.GetStatus().GetConsecutiveFailuresCounter()
                );

                if (auto res = removeScheduledAction(); !res) {
                    return res.Error();
                }

                return false;
            }

            if (scheduledAction.GetStatus().GetLastAttemptStatus().GetStatus() == NMatrix::NApi::TScheduledActionStatus::TAttemptStatus::SUCCESS) {
                Metrics_.PushRate("scheduled_action_with_send_once_policy_succeed");
                logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerScheduledActionWithSendOncePolicySucceed>(
                    scheduledAction.GetMeta().GetId(),
                    scheduledAction.GetMeta().GetGuid()
                );

                if (auto res = removeScheduledAction(); !res) {
                    return res.Error();
                }

                return false;
            }

            break;
        }
        case NMatrix::NApi::TScheduledActionSpec::TSendPolicy::kSendPeriodicallyPolicy: {
            // TODO(chegoryu) Handle SendPeriodicallyPolicy retry policy
            break;
        }
        default: {
            // Nothing to do
            break;
        }
    }

    return true;
}

TExpected<void, TString> TWorkerSync::UpdateScheduledActionStatusAndRescheduleProcessingActionRow(
    const NMatrix::NApi::TScheduledAction& scheduledAction,
    ui64 oldProcessingActionRowShardId,
    TInstant oldProcessingActionRowScheduledAt,
    TInstant oldProcessingActionRowAddedToIncomingQueueAt,
    TString oldProcessingActionRowMovedFromIncomingToProcessingQueueBySyncWithGuid,
    TInstant oldProcessingActionRowMovedFromIncomingToProcessingQueueAt,
    TLogContext logContext
) {
    logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerScheduledActionReschedule>(
        scheduledAction.GetMeta().GetId(),
        scheduledAction.GetMeta().GetGuid(),
        ToString(NProtoInterop::CastFromProto(scheduledAction.GetStatus().GetScheduledAt()))
    );

    return WorkerStorage_.UpdateScheduledActionStatusAndRescheduleProcessingActionRow(
        scheduledAction,
        oldProcessingActionRowShardId,
        oldProcessingActionRowScheduledAt,
        oldProcessingActionRowAddedToIncomingQueueAt,
        oldProcessingActionRowMovedFromIncomingToProcessingQueueBySyncWithGuid,
        oldProcessingActionRowMovedFromIncomingToProcessingQueueAt,
        SyncGuid_,
        logContext,
        Metrics_
    ).GetValueSync();
}

TExpected<void, TString> TWorkerSync::DoAction(
    const NMatrix::NApi::TScheduledActionMeta& meta,
    const NMatrix::NApi::TScheduledActionStatus& status,
    const NMatrix::NApi::TAction& action,
    TLogContext logContext
) {
    auto actionTypeCase = action.GetActionTypeCase();
    {
        TString actionTypeCaseStr = "NotSet";
        if (actionTypeCase != NMatrix::NApi::TAction::ACTIONTYPE_NOT_SET) {
            const auto* fieldDescriptor = NMatrix::NApi::TAction::descriptor()->FindFieldByNumber(actionTypeCase);
            actionTypeCaseStr = fieldDescriptor ? fieldDescriptor->name() : "Unknown";
        }

        logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerDoActionStart>(
            meta.GetId(),
            meta.GetGuid(),
            actionTypeCaseStr
        );
    }

    switch (actionTypeCase) {
        case NMatrix::NApi::TAction::kMockAction: {
            return DoMockAction(meta, status, action.GetMockAction(), logContext);
        }
        case NMatrix::NApi::TAction::kSendTechnicalPush: {
            return DoSendTechnicalPush(meta, action.GetSendTechnicalPush(), logContext);
        }
        default: {
            static const TString error = "Unknown action type";
            return error;
        }
    }
}

TExpected<void, TString> TWorkerSync::DoMockAction(
    const NMatrix::NApi::TScheduledActionMeta& meta,
    const NMatrix::NApi::TScheduledActionStatus& status,
    const NMatrix::NApi::TAction::TMockAction& mockAction,
    TLogContext logContext
) {
    TRtLogActivation rtLogActivation =
        logContext.RtLogPtr()
        ? TRtLogActivation(
            logContext.RtLogPtr(),
            TString::Join(
                "mock-action-",
                meta.GetId(), '-',
                meta.GetGuid()
            ),
            /* newRequest = */ false
        )
        : TRtLogActivation()
    ;
    logContext = TLogContext(
        logContext.FramePtr(),
        RtLogClient_.CreateRequestLogger(rtLogActivation.Token())
    );

    if (status.GetConsecutiveFailuresCounter() < mockAction.GetFailUntilConsecutiveFailuresCounterLessThan()) {
        const TString error = TString::Join(
            "Mock action failed: ConsecutiveFailuresCounter is ", ToString(status.GetConsecutiveFailuresCounter()),
            ", FailUntilConsecutiveFailuresCounterLessThan is ", ToString(mockAction.GetFailUntilConsecutiveFailuresCounterLessThan())
        );

        Metrics_.PushRate("do_mock_action", "error");
        logContext.LogEventErrorCombo<NEvClass::TMatrixWorkerDoMockActionError>(
            meta.GetId(),
            meta.GetGuid(),
            mockAction.GetName(),
            mockAction.GetFailUntilConsecutiveFailuresCounterLessThan(),
            status.GetConsecutiveFailuresCounter()
        );
        rtLogActivation.Finish(/* ok = */ false, error);

        return error;
    } else {
        Metrics_.PushRate("do_mock_action", "ok");
        logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerDoMockActionSuccess>(
            meta.GetId(),
            meta.GetGuid(),
            mockAction.GetName(),
            mockAction.GetFailUntilConsecutiveFailuresCounterLessThan(),
            status.GetConsecutiveFailuresCounter()
        );
        rtLogActivation.Finish(/* ok = */ true);

        return TExpected<void, TString>::DefaultSuccess();
    }

}

TExpected<void, TString> TWorkerSync::DoSendTechnicalPush(
    const NMatrix::NApi::TScheduledActionMeta& meta,
    const NMatrix::NApi::TAction::TSendTechnicalPush& sendTechnicalPush,
    TLogContext logContext
) {
    // TODO(ZION-217) Move this code to other place
    // Unfortunately, now it was necessary to write this asap here

    TRtLogActivation rtLogActivation =
        logContext.RtLogPtr()
        ? TRtLogActivation(
            logContext.RtLogPtr(),
            TString::Join(
                "send-technical-push-",
                meta.GetId(), '-',
                meta.GetGuid()
            ),
            /* newRequest = */ false
        )
        : TRtLogActivation()
    ;

    const TString currentPushId = TString::Join(
        sendTechnicalPush.GetTechnicalPush().GetTechnicalPushId(), '$',
        SyncGuid_, '$',
        CreateGuidAsString()
    );

    logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerDoSendTechnicalPushStart>(
        meta.GetId(),
        meta.GetGuid(),
        MatrixNotificatorClient_.GetHost(),
        MatrixNotificatorClient_.GetPort(),
        sendTechnicalPush.GetUserDeviceIdentifier().GetPuid(),
        sendTechnicalPush.GetUserDeviceIdentifier().GetDeviceId(),
        currentPushId
    );

    NMatrix::NApi::TDelivery request;
    {
        request.SetPuid(sendTechnicalPush.GetUserDeviceIdentifier().GetPuid());
        request.SetDeviceId(sendTechnicalPush.GetUserDeviceIdentifier().GetDeviceId());
        request.SetPushId(currentPushId);
        request.SetSpeechKitDirective(sendTechnicalPush.GetTechnicalPush().GetSpeechKitDirective().value());
    }

    auto fillCommonEventFields = [&meta, &sendTechnicalPush, &currentPushId] <typename T> (
        T& event,
        const TNotificatorResponseRef& response,
        TKeepAliveHttpClient::THttpCode httpResponseCode
    ) {
        event.SetActionId(meta.GetId());
        event.SetActionGuid(meta.GetGuid());
        event.SetPuid(sendTechnicalPush.GetUserDeviceIdentifier().GetPuid());
        event.SetDeviceId(sendTechnicalPush.GetUserDeviceIdentifier().GetDeviceId());
        event.SetPushId(currentPushId);
        if (std::holds_alternative<std::reference_wrapper<const NMatrix::NApi::TDeliveryResponse>>(response)) {
            event.MutableMatrixNotificatorProtoResponse()->CopyFrom(std::get<std::reference_wrapper<const NMatrix::NApi::TDeliveryResponse>>(response));
        } else {
            event.SetMatrixNotificatorUnparsedRawResponse(std::get<std::reference_wrapper<const TString>>(response));
        }
        event.SetMatrixNotificatorHttpResponseCode(httpResponseCode);
    };

    TKeepAliveHttpClient::THttpCode httpResponseCode;
    TStringStream rawResponse;
    try {
        httpResponseCode = MatrixNotificatorClient_.DoRequest(
            "POST",
            "/delivery/push",
            request.SerializeAsString(),
            &rawResponse,
            {
                {"X-RTLog-Token", rtLogActivation.Token()},
            }
        );
    } catch (...) {
        const auto error = TString::Join("Failed to send request: ", CurrentExceptionMessage());
        static const TString emptyResponse = "";

        Metrics_.PushRate("do_send_technical_push", "error");

        NEvClass::TMatrixWorkerDoSendTechnicalPushError event;
        fillCommonEventFields(event, TNotificatorResponseRef(emptyResponse), httpResponseCode);
        event.SetErrorMessage(error);
        logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerDoSendTechnicalPushError>(event);

        rtLogActivation.Finish(/* ok = */ false, error);

        return error;
    }

    NMatrix::NApi::TDeliveryResponse protoResponse;
    bool protoResponseParseOk = protoResponse.ParseFromString(rawResponse.Str());
    const TNotificatorResponseRef response = protoResponseParseOk
        ? TNotificatorResponseRef(protoResponse)
        : TNotificatorResponseRef(rawResponse.Str())
    ;

    bool isOk = (
        protoResponseParseOk &&
        (httpResponseCode >= 200 && httpResponseCode < 300) &&
        protoResponse.GetAddPushToDatabaseStatus().GetStatus() == NMatrix::NApi::TDeliveryResponse::TAddPushToDatabaseStatus::OK &&
        protoResponse.GetSubwayRequestStatus().GetStatus() == NMatrix::NApi::TDeliveryResponse::TSubwayRequestStatus::OK
    );
    bool isDeviceNotConnected = (
        protoResponseParseOk &&
        ((httpResponseCode >= 200 && httpResponseCode < 300) || httpResponseCode == 404) &&
        protoResponse.GetAddPushToDatabaseStatus().GetStatus() == NMatrix::NApi::TDeliveryResponse::TAddPushToDatabaseStatus::OK &&
        (
            protoResponse.GetSubwayRequestStatus().GetStatus() == NMatrix::NApi::TDeliveryResponse::TSubwayRequestStatus::LOCATION_NOT_FOUND ||
            protoResponse.GetSubwayRequestStatus().GetStatus() == NMatrix::NApi::TDeliveryResponse::TSubwayRequestStatus::OUTDATED_LOCATION
        )
    );


    Metrics_.RateHttpCode(1, httpResponseCode, "matrix_notificator");
    if (isOk) {
        Metrics_.PushRate("do_send_technical_push", "ok");

        NEvClass::TMatrixWorkerDoSendTechnicalPushSuccess event;
        fillCommonEventFields(event, response, httpResponseCode);
        logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerDoSendTechnicalPushSuccess>(event);

        rtLogActivation.Finish(/* ok = */ true);

        return TExpected<void, TString>::DefaultSuccess();
    } else if (isDeviceNotConnected) {
        static const TString error = "Device not connected";

        Metrics_.PushRate("do_send_technical_push", "device_not_connected");

        NEvClass::TMatrixWorkerDoSendTechnicalPushDeviceNotConnected event;
        fillCommonEventFields(event, response, httpResponseCode);
        logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerDoSendTechnicalPushDeviceNotConnected>(event);

        rtLogActivation.Finish(/* ok = */ false, error);

        return error;
    } else {
        const TString error = TString::Join("Bad http response code: ", ToString(httpResponseCode));

        Metrics_.PushRate("do_send_technical_push", "error");

        NEvClass::TMatrixWorkerDoSendTechnicalPushError event;
        fillCommonEventFields(event, response, httpResponseCode);
        event.SetErrorMessage(error);
        logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerDoSendTechnicalPushError>(event);

        rtLogActivation.Finish(/* ok = */ false, error);

        return error;
    }
}

void TWorkerSync::PrepareScheduledActionStatusForCurrentAttempt(
    NMatrix::NApi::TScheduledAction& scheduledAction,
    TLogContext logContext
) {
    auto& status = *scheduledAction.MutableStatus();

    // Handle interrupted attempt
    if (status.HasCurrentAttemptStatus()) {
        Metrics_.PushRate("interrupted_scheduled_action_attempt_found");
        logContext.LogEventErrorCombo<NEvClass::TMatrixWorkerInterruptedScheduledActionAttemptFound>(
            scheduledAction.GetMeta().GetId(),
            scheduledAction.GetMeta().GetGuid()
        );

        status.SetInterruptedAttemptsCounter(status.GetInterruptedAttemptsCounter() + 1);
        status.SetConsecutiveFailuresCounter(status.GetConsecutiveFailuresCounter() + 1);
        status.MutableCurrentAttemptStatus()->SetStatus(
            NMatrix::NApi::TScheduledActionStatus::TAttemptStatus::ERROR
        );
        status.MutableLastAttemptStatus()->CopyFrom(status.GetCurrentAttemptStatus());

        status.ClearCurrentAttemptStatus();
    }

    // Reschedule after ConsecutiveFailuresCounter update
    TInstant nextScheduledAt = NProtoInterop::CastFromProto(status.GetScheduledAt());
    switch (scheduledAction.GetSpec().GetSendPolicy().GetPolicyCase()) {
        case NMatrix::NApi::TScheduledActionSpec::TSendPolicy::kSendOncePolicy: {
            nextScheduledAt = GetNextRetryTimeBySendOncePolicy(
                TInstant::Now(),
                scheduledAction.GetSpec().GetSendPolicy().GetSendOncePolicy(),
                status.GetConsecutiveFailuresCounter()
            );
            break;
        }
        case NMatrix::NApi::TScheduledActionSpec::TSendPolicy::kSendPeriodicallyPolicy: {
            nextScheduledAt = GetNextRetryTimeBySendPeriodicallyPolicy(
                TInstant::Now(),
                scheduledAction.GetSpec().GetSendPolicy().GetSendPeriodicallyPolicy(),
                status.GetConsecutiveFailuresCounter()
            );
            break;
        }
        default: {
            // This case should be unreachable due to validations above
            break;
        }
    }
    status.MutableScheduledAt()->CopyFrom(NProtoInterop::CastToProto(nextScheduledAt));

    status.MutableCurrentAttemptStatus()->SetStatus(
        NMatrix::NApi::TScheduledActionStatus::TAttemptStatus::ATTEMPT_STARTED
    );
}

void TWorkerSync::UpdateScheduledActionStatusAfterDoAction(
    NMatrix::NApi::TScheduledAction& scheduledAction,
    const TExpected<void, TString>& doActionRes
) {
    auto& status = *scheduledAction.MutableStatus();

    // Reschedule before ConsecutiveFailuresCounter update
    TInstant nextScheduledAt = NProtoInterop::CastFromProto(status.GetScheduledAt());
    switch (scheduledAction.GetSpec().GetSendPolicy().GetPolicyCase()) {
        case NMatrix::NApi::TScheduledActionSpec::TSendPolicy::kSendOncePolicy: {
            if (!doActionRes) {
                nextScheduledAt = GetNextRetryTimeBySendOncePolicy(
                    TInstant::Now(),
                    scheduledAction.GetSpec().GetSendPolicy().GetSendOncePolicy(),
                    status.GetConsecutiveFailuresCounter()
                );
            }
            break;
        }
        case NMatrix::NApi::TScheduledActionSpec::TSendPolicy::kSendPeriodicallyPolicy: {
            if (doActionRes) {
                nextScheduledAt = GetNextTimeBySendPeriodicallyPolicy(
                    TInstant::Now(),
                    scheduledAction.GetSpec().GetSendPolicy().GetSendPeriodicallyPolicy()
                );
            } else {
                nextScheduledAt = GetNextRetryTimeBySendPeriodicallyPolicy(
                    TInstant::Now(),
                    scheduledAction.GetSpec().GetSendPolicy().GetSendPeriodicallyPolicy(),
                    status.GetConsecutiveFailuresCounter()
                );
            }
            break;
        }
        default: {
            // This case should be unreachable due to validations above
            break;
        }
    }
    status.MutableScheduledAt()->CopyFrom(NProtoInterop::CastToProto(nextScheduledAt));

    if (doActionRes) {
        status.SetSuccessfulAttemptsCounter(status.GetSuccessfulAttemptsCounter() + 1);
        status.SetConsecutiveFailuresCounter(0);

        status.MutableCurrentAttemptStatus()->SetStatus(
            NMatrix::NApi::TScheduledActionStatus::TAttemptStatus::SUCCESS
        );
    } else {
        status.SetFailedAttemptsCounter(status.GetFailedAttemptsCounter() + 1);
        status.SetConsecutiveFailuresCounter(status.GetConsecutiveFailuresCounter() + 1);

        status.MutableCurrentAttemptStatus()->SetStatus(
            NMatrix::NApi::TScheduledActionStatus::TAttemptStatus::ERROR
        );
        status.MutableCurrentAttemptStatus()->SetErrorMessage(doActionRes.Error());
    }

    status.MutableLastAttemptStatus()->CopyFrom(status.GetCurrentAttemptStatus());
    status.ClearCurrentAttemptStatus();
}

} // namespace NMatrix::NWorker
