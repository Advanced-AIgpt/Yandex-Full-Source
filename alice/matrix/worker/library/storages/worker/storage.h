#pragma once

#include <alice/matrix/library/ydb/storage.h>

#include <alice/protos/api/matrix/schedule_action.pb.h>
#include <alice/protos/api/matrix/scheduled_action.pb.h>


namespace NMatrix::NWorker {

class TWorkerStorage : public IYDBStorage {
public:
    struct TShardLockInfo {
        ui64 ShardId;

        bool Locked;
        TString LastLockGuid;
        TString LastLockedBy;

        TInstant LastProcessingStartAt;
        TInstant LastHeartbeatAt;
    };

    struct TAcquireShardLockResult {
        enum class EStatus {
            OK = 0,
            NO_FREE_SHARD = 1,
            TRANSACTION_LOCKS_INVALIDATED = 2,
        };

        EStatus Status;
        TMaybe<TShardLockInfo> ShardLockInfo;
    };

    struct TIncomingActionRow {
        ui64 ShardId;
        TInstant CreatedAt;
        TString ActionId;
        TString ActionGuid;

        TInstant ScheduledAt;
    };

    struct TProcessingActionRow {
        ui64 ShardId;
        TInstant ScheduledAt;
        TString ActionId;
        TString ActionGuid;

        TInstant AddedToIncomingQueueAt;

        TString MovedFromIncomingToProcessingQueueBySyncWithGuid;
        TInstant MovedFromIncomingToProcessingQueueAt;

        TString LastRescheduleBySyncWithGuid;
        TInstant LastRescheduleAt;
    };

public:
    TWorkerStorage(
        const NYdb::TDriver& driver,
        const TYDBClientSettings& config
    );

    // Returns Nothing() if there are no free shard lock
    // otherwise returns shard lock info BEFORE shard lock was acquired
    NThreading::TFuture<TExpected<TAcquireShardLockResult, TString>> AcquireShardLock(
        const TString& lockGuid,
        const TDuration maxHeartbeatInactivityPeriodToAcquireLock,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<void, TString>> ReleaseShardLock(
        ui64 shardId,
        const TString lockGuid,
        const TDuration maxHeartbeatInactivityPeriodToReleaseLock,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<void, TString>> EnsureShardLockLeadingAndDoHeartbeat(
        ui64 shardId,
        const TString& lockGuid,
        const TDuration maxHeartbeatInactivityPeriodToEnsureLeading,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<TVector<TIncomingActionRow>, TString>> GetIncomingActionRows(
        ui64 shardId,
        const ui64 limit,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<void, TString>> MoveActionRowFromIncomingToProcessing(
        const TIncomingActionRow& incomingActionRow,
        const TString& syncGuid,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<TVector<TProcessingActionRow>, TString>> GetProcessingActionRows(
        ui64 shardId,
        const ui64 limit,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<void, TString>> RemoveProcessingActionRow(
        const TProcessingActionRow& processingActionRow,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<void, TString>> RemoveScheduledActionWithProcessingActionRow(
        const NMatrix::NApi::TScheduledAction& scheduledAction,
        ui64 processingActionRowShardId,
        TInstant processingActionRowScheduledAt,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<void, TString>> UpdateScheduledActionStatusAndRescheduleProcessingActionRow(
        const NMatrix::NApi::TScheduledAction& scheduledAction,
        ui64 oldProcessingActionRowShardId,
        TInstant oldProcessingActionRowScheduledAt,
        TInstant oldProcessingActionRowAddedToIncomingQueueAt,
        TString oldProcessingActionRowMovedFromIncomingToProcessingQueueBySyncWithGuid,
        TInstant oldProcessingActionRowMovedFromIncomingToProcessingQueueAt,
        const TString& syncGuid,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<TMaybe<NMatrix::NApi::TScheduledAction>, TString>> GetScheduledAction(
        const TString& actionId,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

private:
    static inline constexpr TStringBuf NAME = "worker";
};

} // namespace NMatrix::NWorker
