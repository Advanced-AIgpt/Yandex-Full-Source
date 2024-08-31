#pragma once

#include <alice/matrix/worker/library/storages/worker/storage.h>

#include <alice/protos/api/matrix/action.pb.h>

#include <library/cpp/http/simple/http_client.h>


namespace NMatrix::NWorker {

class TWorkerSync {
public:
    enum class ESyncStatus {
        PERFORMED = 0,
        SKIPPED = 1,
    };

private:
    enum class EPerfromActionStatus {
        SUCCESS = 0,
        DO_ACTION_ERROR = 1,
    };

public:
    TWorkerSync(
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
    );
    ~TWorkerSync();

    TExpected<ESyncStatus, TString> Run(TLogContext logContext);

private:
    void InitMetrics();

    TExpected<void, TString> EnsureShardLockLeading(
        ui64 shardId,
        TLogContext logContext
    );

    TExpected<void, TString> MoveActionRowsFromIncomingToProcessing(
        ui64 shardId,
        TLogContext logContext
    );
    TExpected<void, TString> MoveActionRowFromIncomingToProcessing(
        const TWorkerStorage::TIncomingActionRow& incomingActionRow,
        TLogContext logContext
    );

    TExpected<void, TString> PerformActionsFromProcessing(
        ui64 shardId,
        TLogContext logContext
    );
    TExpected<EPerfromActionStatus, TString> PerformActionFromProcessing(
        const TWorkerStorage::TProcessingActionRow& processingActionRow,
        TLogContext logContext
    );

    TExpected<TMaybe<NMatrix::NApi::TScheduledAction>, TString> GetScheduledActionAndRemoveProcessingActionRowIfNeeded(
        const TWorkerStorage::TProcessingActionRow& processingActionRow,
        TLogContext logContext
    );
    TExpected<bool, TString> CheckGuidAndRemoveProcessingActionRowIfNeeded(
        const NMatrix::NApi::TScheduledAction& scheduledAction,
        const TWorkerStorage::TProcessingActionRow& processingActionRow,
        TLogContext logContext
    );
    TExpected<bool, TString> CheckDeadlineAndRemoveScheduledActionIfNeeded(
        const NMatrix::NApi::TScheduledAction& scheduledAction,
        ui64 processingActionRowShardId,
        TInstant processingActionRowScheduledAt,
        TLogContext logContext
    );
    TExpected<bool, TString> CheckStatusAndRemoveScheduledActionIfNeeded(
        const NMatrix::NApi::TScheduledAction& scheduledAction,
        ui64 processingActionRowShardId,
        TInstant processingActionRowScheduledAt,
        TLogContext logContext
    );
    TExpected<void, TString> UpdateScheduledActionStatusAndRescheduleProcessingActionRow(
        const NMatrix::NApi::TScheduledAction& scheduledAction,
        ui64 oldProcessingActionRowShardId,
        TInstant oldProcessingActionRowScheduledAt,
        TInstant oldProcessingActionRowAddedToIncomingQueueAt,
        TString oldProcessingActionRowMovedFromIncomingToProcessingQueueBySyncWithGuid,
        TInstant oldProcessingActionRowMovedFromIncomingToProcessingQueueAt,
        TLogContext logContext
    );

    TExpected<void, TString> DoAction(
        const NMatrix::NApi::TScheduledActionMeta& meta,
        const NMatrix::NApi::TScheduledActionStatus& status,
        const NMatrix::NApi::TAction& action,
        TLogContext logContext
    );
    TExpected<void, TString> DoMockAction(
        const NMatrix::NApi::TScheduledActionMeta& meta,
        const NMatrix::NApi::TScheduledActionStatus& status,
        const NMatrix::NApi::TAction::TMockAction& mockAction,
        TLogContext logContext
    );
    TExpected<void, TString> DoSendTechnicalPush(
        const NMatrix::NApi::TScheduledActionMeta& meta,
        const NMatrix::NApi::TAction::TSendTechnicalPush& sendTechnicalPush,
        TLogContext logContext
    );

    void PrepareScheduledActionStatusForCurrentAttempt(
        NMatrix::NApi::TScheduledAction& scheduledAction,
        TLogContext logContext
    );
    void UpdateScheduledActionStatusAfterDoAction(
        NMatrix::NApi::TScheduledAction& scheduledAction,
        const TExpected<void, TString>& doActionRes
    );

private:
    static inline constexpr TStringBuf NAME = "worker_sync";
    static std::once_flag INIT_METRICS_ONCE_FLAG;

    TWorkerStorage& WorkerStorage_;
    TRtLogClient& RtLogClient_;
    TKeepAliveHttpClient& MatrixNotificatorClient_;

    TInstant StartTime_;
    TSourceMetrics Metrics_;

    const TString SyncGuid_;
    const ui64 SelectLimit_;
    const TDuration PerActionTimeout_;
    const TDuration MaxHeartbeatInactivityPeriodToAcquireLock_;
    const TDuration MaxHeartbeatInactivityPeriodToReleaseLock_;
    const TDuration MaxHeartbeatInactivityPeriodToEnsureLeading_;
    const TDuration MinEnsureShardLockLeadingAndDoHeartbeatPeriod_;

    TMaybe<TString> EnsureShardLockLeadingFailedWithError_;
    TInstant LastEnsureShardLockLeadingAt_;
};

} // namespace NMatrix::NWorker
