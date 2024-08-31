#include "storage.h"

#include <alice/matrix/library/version/version.h>

#include <library/cpp/protobuf/interop/cast.h>

#include <util/digest/city.h>
#include <util/system/hostname.h>

#include <sstream>
#include <thread>

namespace NMatrix::NWorker {

namespace {

TString GetThisThreadIdStr() {
    // Do not use TThread::CurrentThreadId() here because this function hashes the thread id
    // https://a.yandex-team.ru/arc/trunk/arcadia/util/system/thread.i?rev=r9396278#L32-43

    // Arcadia string utils do not work this std::this_thread::get_id()
    // https://st.yandex-team.ru/IGNIETFERRO-1904#611bc405a15e4e20890bb93e
    std::stringstream ss;
    ss << std::hex << std::showbase;
    ss << std::this_thread::get_id();
    return ss.str();
}

TString GetHostNameSafe() {
    try {
        return HostName();
    } catch (...) {
        return "unknown_hostname";
    }
}

TString GetSelfName() {
    static thread_local TString selfName = TString::Join(
        GetHostNameSafe(),
        '/', GetVersion(),
        '/', GetThisThreadIdStr()
    );

    return selfName;
}

} // namespace

TWorkerStorage::TWorkerStorage(
    const NYdb::TDriver& driver,
    const TYDBClientSettings& config
)
    : IYDBStorage(driver, config, NAME)
{}

NThreading::TFuture<TExpected<TWorkerStorage::TAcquireShardLockResult, TString>> TWorkerStorage::AcquireShardLock(
    const TString& lockGuid,
    const TDuration maxHeartbeatInactivityPeriodToAcquireLock,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName =  "acquire_shard_lock";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, lockGuid, maxHeartbeatInactivityPeriodToAcquireLock, selfName = GetSelfName(), resultSet](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $lock_guid AS String;
            DECLARE $self_name AS String;

            DECLARE $now AS Timestamp;
            DECLARE $min_allowed_heartbeat_time_for_acquired_lock AS Timestamp;

            $free_shard = (
                SELECT *
                FROM
                    shard_locks
                WHERE
                    NOT locked OR (locked AND last_heartbeat_at < $min_allowed_heartbeat_time_for_acquired_lock)
                ORDER BY last_processing_start_at
                LIMIT 1
            );

            SELECT * FROM $free_shard;

            UPDATE shard_locks ON
            SELECT
                shard_id,

                True AS locked,
                $lock_guid AS last_lock_guid,
                $self_name AS last_locked_by,

                $now AS last_processing_start_at,
                $now AS last_heartbeat_at,
            FROM $free_shard;
        )";

        const auto now = TInstant::Now();
        auto params = session.GetParamsBuilder()
            .AddParam("$lock_guid")
                .String(lockGuid)
                .Build()
            .AddParam("$self_name")
                .String(selfName)
                .Build()
            .AddParam("$now")
                .Timestamp(now)
                .Build()
            .AddParam("$min_allowed_heartbeat_time_for_acquired_lock")
                .Timestamp(now - maxHeartbeatInactivityPeriodToAcquireLock)
                .Build()
            .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            std::move(params),
            ExecDataQuerySettings_
        ).Apply([resultSet](const NYdb::NTable::TAsyncDataQueryResult& fut) mutable -> NYdb::TStatus {
            const auto& res = fut.GetValueSync();
            if (res.IsSuccess()) {
                *resultSet = res.GetResultSet(0);
            }

            return res;
        });
    }, RetryOperationSettings_).Apply(
        [resultSet, operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<TAcquireShardLockResult, TString> {
            static IYDBStorage::TResultChecker resultChecker = [](const NYdb::TStatus& res) -> TExpected<void, TString> {
                if (res.IsSuccess()) {
                    return TExpected<void, TString>::DefaultSuccess();
                } else {
                    const TString error = IYDBStorage::YDBStatusToString(res);
                    // There is no better way to check this error than to check the status and substring of the error
                    // Just in case, error looks like this "ABORTED\n<main>: Error: Execution, code: 1060\n    <main>: Error: Transaction locks invalidated. Table: /ru/speechkit_ops_alice_notificator/prod/matrix-queue-common/shard_locks, code: 2001"

                    if (res.GetStatus() == NYdb::EStatus::ABORTED && error.find("Transaction locks invalidated.") != TString::npos) {
                        return TExpected<void, TString>::DefaultSuccess();
                    } else {
                        return error;
                    }
                }
            };

            const auto& status = fut.GetValueSync();
            if (const auto opRes = operationContext.ReportResult(status, resultChecker); !opRes) {
                return opRes.Error();
            }

            // Custom checker handles all errors correctly, so if the operation failed and we are here, it means we have "Transaction locks invalidated"
            if (!status.IsSuccess()) {
                return TAcquireShardLockResult({
                    .Status = TAcquireShardLockResult::EStatus::TRANSACTION_LOCKS_INVALIDATED,
                    .ShardLockInfo = Nothing(),
                });
            }

            NYdb::TResultSetParser parser(resultSet->GetRef());
            if (parser.TryNextRow()) {
                return TAcquireShardLockResult({
                    .Status = TAcquireShardLockResult::EStatus::OK,
                    .ShardLockInfo = TMaybe<TShardLockInfo>(TShardLockInfo({
                        .ShardId = *parser.ColumnParser("shard_id").GetOptionalUint64(),

                        .Locked = *parser.ColumnParser("locked").GetOptionalBool(),
                        .LastLockGuid = *parser.ColumnParser("last_lock_guid").GetOptionalString(),
                        .LastLockedBy = *parser.ColumnParser("last_locked_by").GetOptionalString(),

                        .LastProcessingStartAt = *parser.ColumnParser("last_processing_start_at").GetOptionalTimestamp(),
                        .LastHeartbeatAt = *parser.ColumnParser("last_heartbeat_at").GetOptionalTimestamp(),
                    })),
                });
            } else {
                return TAcquireShardLockResult({
                    .Status = TAcquireShardLockResult::EStatus::NO_FREE_SHARD,
                    .ShardLockInfo = Nothing(),
                });
            }
        }
    );
}

NThreading::TFuture<TExpected<void, TString>> TWorkerStorage::ReleaseShardLock(
    ui64 shardId,
    const TString lockGuid,
    const TDuration maxHeartbeatInactivityPeriodToReleaseLock,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "release_shard_lock";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, shardId, lockGuid, maxHeartbeatInactivityPeriodToReleaseLock](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_id AS Uint64;
            DECLARE $lock_guid AS String;

            DECLARE $min_allowed_heartbeat_time_to_release_lock AS Timestamp;

            DISCARD SELECT Ensure(
                0,
                last_lock_guid == $lock_guid,
                "Lock guid changed: expected: "
                    || $lock_guid
                    || ", got: "
                    || Unwrap(last_lock_guid, "NULL")
                    || ". Lock owner: " || Unwrap(last_locked_by, "NULL")
            ) FROM shard_locks WHERE shard_id = $shard_id;

            DISCARD SELECT Ensure(
                0,
                last_heartbeat_at >= $min_allowed_heartbeat_time_to_release_lock,
                "Last heartbeat time is too old: min_allowed_heartbeat_time_to_release_lock: "
                    || CAST($min_allowed_heartbeat_time_to_release_lock AS String)
                    || ", last_heartbeat_at: "
                    || Unwrap(CAST(last_heartbeat_at AS String), "NULL")
            ) FROM shard_locks WHERE shard_id = $shard_id;

            UPDATE shard_locks SET locked = false
            WHERE shard_id = $shard_id;
        )";

        const auto now = TInstant::Now();
        auto params = session.GetParamsBuilder()
            .AddParam("$shard_id")
                .Uint64(shardId)
                .Build()
            .AddParam("$lock_guid")
                .String(lockGuid)
                .Build()
            .AddParam("$min_allowed_heartbeat_time_to_release_lock")
                .Timestamp(now - maxHeartbeatInactivityPeriodToReleaseLock)
                .Build()
            .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            std::move(params),
            ExecDataQuerySettings_
        ).Apply([](const NYdb::NTable::TAsyncDataQueryResult& fut) mutable -> NYdb::TStatus {
            return fut.GetValueSync();
        });
    }, RetryOperationSettings_).Apply(
        [operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<void, TString> {
            return operationContext.ReportResult(fut.GetValueSync());
        }
    );
}

NThreading::TFuture<TExpected<void, TString>> TWorkerStorage::EnsureShardLockLeadingAndDoHeartbeat(
    ui64 shardId,
    const TString& lockGuid,
    const TDuration maxHeartbeatInactivityPeriodToEnsureLeading,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "ensure_shard_lock_leading_and_do_heartbeat";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, shardId, lockGuid, maxHeartbeatInactivityPeriodToEnsureLeading](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_id AS Uint64;
            DECLARE $lock_guid AS String;

            DECLARE $now AS Timestamp;
            DECLARE $min_allowed_heartbeat_time_to_ensure_leading AS Timestamp;

            DISCARD SELECT Ensure(
                0,
                last_lock_guid == $lock_guid,
                "Lock guid changed: expected: "
                    || $lock_guid
                    || ", got: "
                    || Unwrap(last_lock_guid, "NULL")
                    || ". Lock owner: " || Unwrap(last_locked_by, "NULL")
            ) FROM shard_locks WHERE shard_id = $shard_id;

            DISCARD SELECT Ensure(
                0,
                last_heartbeat_at >= $min_allowed_heartbeat_time_to_ensure_leading,
                "Last heartbeat time is too old: min_allowed_heartbeat_time_to_ensure_leading: "
                    || CAST($min_allowed_heartbeat_time_to_ensure_leading AS String)
                    || ", last_heartbeat_at: "
                    || Unwrap(CAST(last_heartbeat_at AS String), "NULL")
            ) FROM shard_locks WHERE shard_id = $shard_id;

            UPDATE shard_locks SET last_heartbeat_at = $now
            WHERE shard_id = $shard_id;
        )";

        const auto now = TInstant::Now();
        auto params = session.GetParamsBuilder()
            .AddParam("$shard_id")
                .Uint64(shardId)
                .Build()
            .AddParam("$lock_guid")
                .String(lockGuid)
                .Build()
            .AddParam("$now")
                .Timestamp(now)
                .Build()
            .AddParam("$min_allowed_heartbeat_time_to_ensure_leading")
                .Timestamp(now - maxHeartbeatInactivityPeriodToEnsureLeading)
                .Build()
            .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            std::move(params),
            ExecDataQuerySettings_
        ).Apply([](const NYdb::NTable::TAsyncDataQueryResult& fut) mutable -> NYdb::TStatus {
            return fut.GetValueSync();
        });
    }, RetryOperationSettings_).Apply(
        [operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<void, TString> {
            return operationContext.ReportResult(fut.GetValueSync());
        }
    );
}

NThreading::TFuture<TExpected<TVector<TWorkerStorage::TIncomingActionRow>, TString>> TWorkerStorage::GetIncomingActionRows(
    ui64 shardId,
    const ui64 limit,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "get_incoming_action_rows";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, shardId, limit, resultSet](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_id AS Uint64;
            DECLARE $limit AS Uint64;

            SELECT
                shard_id,
                created_at,
                action_id,
                action_guid,

                scheduled_at

            FROM incoming_queue
            WHERE
                shard_id = $shard_id
            ORDER BY shard_id, created_at
            LIMIT $limit;
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_id")
                .Uint64(shardId)
                .Build()
            .AddParam("$limit")
                .Uint64(limit)
                .Build()
            .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::OnlineRO()).CommitTx(),
            std::move(params),
            ExecDataQuerySettings_
        ).Apply([resultSet](const NYdb::NTable::TAsyncDataQueryResult& fut) mutable -> NYdb::TStatus {
            const auto& res = fut.GetValueSync();
            if (res.IsSuccess()) {
                *resultSet = res.GetResultSet(0);
            }

            return res;
        });
    }, RetryOperationSettings_).Apply(
        [resultSet, operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<TVector<TWorkerStorage::TIncomingActionRow>, TString> {
            const auto& status = fut.GetValueSync();
            if (const auto opRes = operationContext.ReportResult(status); !opRes) {
                return opRes.Error();
            }

            NYdb::TResultSetParser parser(resultSet->GetRef());
            TVector<TWorkerStorage::TIncomingActionRow> incomingActionRows;
            incomingActionRows.reserve(parser.RowsCount());
            while (parser.TryNextRow()) {
                incomingActionRows.emplace_back(TIncomingActionRow({
                    .ShardId = *parser.ColumnParser("shard_id").GetOptionalUint64(),
                    .CreatedAt = *parser.ColumnParser("created_at").GetOptionalTimestamp(),
                    .ActionId = *parser.ColumnParser("action_id").GetOptionalString(),
                    .ActionGuid = *parser.ColumnParser("action_guid").GetOptionalString(),

                    .ScheduledAt = *parser.ColumnParser("scheduled_at").GetOptionalTimestamp(),
                }));
            }

            return incomingActionRows;
        }
    );
}

NThreading::TFuture<TExpected<void, TString>> TWorkerStorage::MoveActionRowFromIncomingToProcessing(
    const TWorkerStorage::TIncomingActionRow& incomingActionRow,
    const TString& syncGuid,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "move_action_row_from_incoming_to_processing";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, incomingActionRow, syncGuid](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_id AS Uint64;
            DECLARE $scheduled_at AS Timestamp;
            DECLARE $action_id AS String;
            DECLARE $action_guid AS String;

            DECLARE $now AS Timestamp;
            DECLARE $added_to_incoming_queue_at AS Timestamp;

            DECLARE $sync_guid AS String;

            DELETE FROM incoming_queue
            WHERE
                shard_id = $shard_id AND
                created_at = $added_to_incoming_queue_at AND
                action_id = $action_id AND
                action_guid = $action_guid
            ;

            UPSERT INTO processing_queue
                (
                    shard_id,
                    scheduled_at,
                    action_id,
                    action_guid,

                    added_to_incoming_queue_at,

                    moved_from_incoming_to_processing_queue_by_sync_with_guid,
                    moved_from_incoming_to_processing_queue_at,

                    last_reschedule_by_sync_with_guid,
                    last_reschedule_at
                )
            VALUES
                (
                    $shard_id,
                    $scheduled_at,
                    $action_id,
                    $action_guid,

                    $added_to_incoming_queue_at,

                    $sync_guid,
                    $now,

                    $sync_guid,
                    $now
                )
            ;
        )";

        const auto now = TInstant::Now();
        auto params = session.GetParamsBuilder()
            .AddParam("$shard_id")
                .Uint64(incomingActionRow.ShardId)
                .Build()
            .AddParam("$scheduled_at")
                .Timestamp(incomingActionRow.ScheduledAt)
                .Build()
            .AddParam("$action_id")
                .String(incomingActionRow.ActionId)
                .Build()
            .AddParam("$action_guid")
                .String(incomingActionRow.ActionGuid)
                .Build()
            .AddParam("$now")
                .Timestamp(now)
                .Build()
            .AddParam("$added_to_incoming_queue_at")
                .Timestamp(incomingActionRow.CreatedAt)
                .Build()
            .AddParam("$sync_guid")
                .String(syncGuid)
                .Build()
            .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            std::move(params),
            ExecDataQuerySettings_
        ).Apply([](const NYdb::NTable::TAsyncDataQueryResult& fut) mutable -> NYdb::TStatus {
            return fut.GetValueSync();
        });
    }, RetryOperationSettings_).Apply(
        [operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<void, TString> {
            return operationContext.ReportResult(fut.GetValueSync());
        }
    );
}

NThreading::TFuture<TExpected<TVector<TWorkerStorage::TProcessingActionRow>, TString>> TWorkerStorage::GetProcessingActionRows(
    ui64 shardId,
    const ui64 limit,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "get_processing_action_rows";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, shardId, limit, resultSet](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_id AS Uint64;
            DECLARE $limit AS Uint64;

            DECLARE $now AS Timestamp;

            SELECT
                shard_id,
                scheduled_at,
                action_id,
                action_guid,

                added_to_incoming_queue_at,

                moved_from_incoming_to_processing_queue_by_sync_with_guid,
                moved_from_incoming_to_processing_queue_at,

                last_reschedule_by_sync_with_guid,
                last_reschedule_at

            FROM processing_queue
            WHERE
                shard_id = $shard_id AND
                scheduled_at <= $now
            ORDER BY shard_id, scheduled_at
            LIMIT $limit;
        )";

        const auto now = TInstant::Now();
        auto params = session.GetParamsBuilder()
            .AddParam("$shard_id")
                .Uint64(shardId)
                .Build()
            .AddParam("$limit")
                .Uint64(limit)
                .Build()
            .AddParam("$now")
                .Timestamp(now)
                .Build()
            .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::OnlineRO()).CommitTx(),
            std::move(params),
            ExecDataQuerySettings_
        ).Apply([resultSet](const NYdb::NTable::TAsyncDataQueryResult& fut) mutable -> NYdb::TStatus {
            const auto& res = fut.GetValueSync();
            if (res.IsSuccess()) {
                *resultSet = res.GetResultSet(0);
            }

            return res;
        });
    }, RetryOperationSettings_).Apply(
        [resultSet, operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<TVector<TWorkerStorage::TProcessingActionRow>, TString> {
            const auto& status = fut.GetValueSync();
            if (const auto opRes = operationContext.ReportResult(status); !opRes) {
                return opRes.Error();
            }

            NYdb::TResultSetParser parser(resultSet->GetRef());
            TVector<TWorkerStorage::TProcessingActionRow> processingActionRows;
            processingActionRows.reserve(parser.RowsCount());
            while (parser.TryNextRow()) {
                processingActionRows.emplace_back(TProcessingActionRow({
                    .ShardId = *parser.ColumnParser("shard_id").GetOptionalUint64(),
                    .ScheduledAt = *parser.ColumnParser("scheduled_at").GetOptionalTimestamp(),
                    .ActionId = *parser.ColumnParser("action_id").GetOptionalString(),
                    .ActionGuid = *parser.ColumnParser("action_guid").GetOptionalString(),

                    .AddedToIncomingQueueAt = *parser.ColumnParser("added_to_incoming_queue_at").GetOptionalTimestamp(),

                    .MovedFromIncomingToProcessingQueueBySyncWithGuid = *parser.ColumnParser("moved_from_incoming_to_processing_queue_by_sync_with_guid").GetOptionalString(),
                    .MovedFromIncomingToProcessingQueueAt = *parser.ColumnParser("moved_from_incoming_to_processing_queue_at").GetOptionalTimestamp(),

                    .LastRescheduleBySyncWithGuid = *parser.ColumnParser("last_reschedule_by_sync_with_guid").GetOptionalString(),
                    .LastRescheduleAt = *parser.ColumnParser("last_reschedule_at").GetOptionalTimestamp(),
                }));
            }

            return processingActionRows;
        }
    );
}

NThreading::TFuture<TExpected<void, TString>> TWorkerStorage::RemoveProcessingActionRow(
    const TWorkerStorage::TProcessingActionRow& processingActionRow,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "remove_processing_action_row";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, processingActionRow](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_id AS Uint64;
            DECLARE $scheduled_at AS Timestamp;
            DECLARE $action_id AS String;
            DECLARE $action_guid AS String;

            DELETE FROM processing_queue
            WHERE
                shard_id = $shard_id AND
                scheduled_at = $scheduled_at AND
                action_id = $action_id AND
                action_guid = $action_guid
            ;
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_id")
                .Uint64(processingActionRow.ShardId)
                .Build()
            .AddParam("$scheduled_at")
                .Timestamp(processingActionRow.ScheduledAt)
                .Build()
            .AddParam("$action_id")
                .String(processingActionRow.ActionId)
                .Build()
            .AddParam("$action_guid")
                .String(processingActionRow.ActionGuid)
                .Build()
            .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            std::move(params),
            ExecDataQuerySettings_
        ).Apply([](const NYdb::NTable::TAsyncDataQueryResult& fut) mutable -> NYdb::TStatus {
            return fut.GetValueSync();
        });
    }, RetryOperationSettings_).Apply(
        [operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<void, TString> {
            return operationContext.ReportResult(fut.GetValueSync());
        }
    );
}

NThreading::TFuture<TExpected<void, TString>> TWorkerStorage::RemoveScheduledActionWithProcessingActionRow(
    const NMatrix::NApi::TScheduledAction& scheduledAction,
    ui64 processingActionRowShardId,
    TInstant processingActionRowScheduledAt,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "remove_scheduled_action_with_processing_action_row";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, scheduledAction, processingActionRowShardId, processingActionRowScheduledAt](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_id AS Uint64;
            DECLARE $processing_action_row_scheduled_at AS Timestamp;
            DECLARE $action_id_hash AS Uint64;
            DECLARE $action_id AS String;
            DECLARE $action_guid AS String;

            DELETE FROM processing_queue
            WHERE
                shard_id = $shard_id AND
                scheduled_at = $processing_action_row_scheduled_at AND
                action_id = $action_id AND
                action_guid = $action_guid
            ;

            -- 'WHERE ... meta_action_guid = $action_guid' works fast because there is at most one row with such $action_id
            -- This condition is needed to prevent race between this removing and action overriding via scheduler
            -- We assume that overriding from scheduler is more important
            DELETE FROM scheduled_actions
            WHERE
                meta_action_id_hash = $action_id_hash AND
                meta_action_id = $action_id AND
                meta_action_guid = $action_guid
            ;
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_id")
                .Uint64(processingActionRowShardId)
                .Build()
            .AddParam("$processing_action_row_scheduled_at")
                // WARNING: scheduledAction status can be already updated at this point
                // So, we can't use Timestamp(NProtoInterop::CastFromProto(scheduledAction.GetStatus().GetScheduledAt()))
                .Timestamp(processingActionRowScheduledAt)
                .Build()
            .AddParam("$action_id_hash")
                .Uint64(CityHash64(scheduledAction.GetMeta().GetId()))
                .Build()
            .AddParam("$action_id")
                .String(scheduledAction.GetMeta().GetId())
                .Build()
            .AddParam("$action_guid")
                .String(scheduledAction.GetMeta().GetGuid())
                .Build()
            .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            std::move(params),
            ExecDataQuerySettings_
        ).Apply([](const NYdb::NTable::TAsyncDataQueryResult& fut) mutable -> NYdb::TStatus {
            return fut.GetValueSync();
        });
    }, RetryOperationSettings_).Apply(
        [operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<void, TString> {
            return operationContext.ReportResult(fut.GetValueSync());
        }
    );
}

NThreading::TFuture<TExpected<void, TString>> TWorkerStorage::UpdateScheduledActionStatusAndRescheduleProcessingActionRow(
    const NMatrix::NApi::TScheduledAction& scheduledAction,
    ui64 oldProcessingActionRowShardId,
    TInstant oldProcessingActionRowScheduledAt,
    TInstant oldProcessingActionRowAddedToIncomingQueueAt,
    TString oldProcessingActionRowMovedFromIncomingToProcessingQueueBySyncWithGuid,
    TInstant oldProcessingActionRowMovedFromIncomingToProcessingQueueAt,
    const TString& syncGuid,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "update_scheduled_action_status_and_reschedule_processing_action_row";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation(
    [
        this,
        scheduledAction,
        oldProcessingActionRowShardId,
        oldProcessingActionRowScheduledAt,
        oldProcessingActionRowAddedToIncomingQueueAt,
        oldProcessingActionRowMovedFromIncomingToProcessingQueueBySyncWithGuid,
        oldProcessingActionRowMovedFromIncomingToProcessingQueueAt,
        syncGuid
    ](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_id AS Uint64;
            DECLARE $scheduled_at AS Timestamp;
            DECLARE $action_id_hash AS Uint64;
            DECLARE $action_id AS String;
            DECLARE $action_guid AS String;

            DECLARE $now AS Timestamp;
            DECLARE $added_to_incoming_queue_at AS Timestamp;
            DECLARE $moved_from_incoming_to_processing_queue_by_sync_with_guid AS String;
            DECLARE $moved_from_incoming_to_processing_queue_at AS Timestamp;

            DECLARE $old_processing_action_row_scheduled_at AS Timestamp;

            DECLARE $sync_guid AS String;

            DECLARE $new_action_status AS String;

            DELETE FROM processing_queue
            WHERE
                shard_id = $shard_id AND
                scheduled_at = $old_processing_action_row_scheduled_at AND
                action_id = $action_id AND
                action_guid = $action_guid AND
                -- Do not delete and upsert same row
                $scheduled_at != $old_processing_action_row_scheduled_at
            ;

            UPSERT INTO processing_queue
                (
                    shard_id,
                    scheduled_at,
                    action_id,
                    action_guid,

                    added_to_incoming_queue_at,

                    moved_from_incoming_to_processing_queue_by_sync_with_guid,
                    moved_from_incoming_to_processing_queue_at,

                    last_reschedule_by_sync_with_guid,
                    last_reschedule_at
                )
            VALUES
                (
                    $shard_id,
                    $scheduled_at,
                    $action_id,
                    $action_guid,

                    $added_to_incoming_queue_at,

                    $moved_from_incoming_to_processing_queue_by_sync_with_guid,
                    $moved_from_incoming_to_processing_queue_at,

                    $sync_guid,
                    $now
                )
            ;

            -- 'WHERE ... meta_action_guid = $action_guid' works fast because there is at most one row with such $action_id
            -- This condition is needed to prevent race between this status update and action overriding via scheduler
            -- We assume that overriding from scheduler is more important
            UPDATE
                scheduled_actions
            SET
                status = $new_action_status,
                status_scheduled_at = $scheduled_at
            WHERE
                meta_action_id_hash = $action_id_hash AND
                meta_action_id = $action_id AND
                meta_action_guid = $action_guid
            ;
        )";

        const auto now = TInstant::Now();
        auto params = session.GetParamsBuilder()
            .AddParam("$shard_id")
                .Uint64(oldProcessingActionRowShardId)
                .Build()
            .AddParam("$scheduled_at")
                .Timestamp(NProtoInterop::CastFromProto(scheduledAction.GetStatus().GetScheduledAt()))
                .Build()
            .AddParam("$action_id_hash")
                .Uint64(CityHash64(scheduledAction.GetMeta().GetId()))
                .Build()
            .AddParam("$action_id")
                .String(scheduledAction.GetMeta().GetId())
                .Build()
            .AddParam("$action_guid")
                .String(scheduledAction.GetMeta().GetGuid())
                .Build()

            .AddParam("$now")
                .Timestamp(now)
                .Build()
            .AddParam("$added_to_incoming_queue_at")
                .Timestamp(oldProcessingActionRowAddedToIncomingQueueAt)
                .Build()
            .AddParam("$moved_from_incoming_to_processing_queue_by_sync_with_guid")
                .String(oldProcessingActionRowMovedFromIncomingToProcessingQueueBySyncWithGuid)
                .Build()
            .AddParam("$moved_from_incoming_to_processing_queue_at")
                .Timestamp(oldProcessingActionRowMovedFromIncomingToProcessingQueueAt)
                .Build()

            .AddParam("$old_processing_action_row_scheduled_at")
                .Timestamp(oldProcessingActionRowScheduledAt)
                .Build()

            .AddParam("$sync_guid")
                .String(syncGuid)
                .Build()

            .AddParam("$new_action_status")
                .String(scheduledAction.GetStatus().SerializeAsString())
                .Build()

            .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            std::move(params),
            ExecDataQuerySettings_
        ).Apply([](const NYdb::NTable::TAsyncDataQueryResult& fut) mutable -> NYdb::TStatus {
            return fut.GetValueSync();
        });
    }, RetryOperationSettings_).Apply(
        [operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<void, TString> {
            return operationContext.ReportResult(fut.GetValueSync());
        }
    );
}

NThreading::TFuture<TExpected<TMaybe<NMatrix::NApi::TScheduledAction>, TString>> TWorkerStorage::GetScheduledAction(
    const TString& actionId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName =  "get_scheduled_action";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, actionId, resultSet](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $action_id_hash AS Uint64;
            DECLARE $action_id AS String;

            SELECT
                meta,
                spec,
                status
            FROM
                scheduled_actions
            WHERE
                meta_action_id_hash = $action_id_hash AND
                meta_action_id = $action_id;
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$action_id_hash")
                .Uint64(CityHash64(actionId))
                .Build()
            .AddParam("$action_id")
                .String(actionId)
                .Build()
            .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::OnlineRO()).CommitTx(),
            std::move(params),
            ExecDataQuerySettings_
        ).Apply([resultSet](const NYdb::NTable::TAsyncDataQueryResult& fut) mutable -> NYdb::TStatus {
            const auto& res = fut.GetValueSync();
            if (res.IsSuccess()) {
                *resultSet = res.GetResultSet(0);
            }

            return res;
        });
    }, RetryOperationSettings_).Apply(
        [resultSet, operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<TMaybe<NMatrix::NApi::TScheduledAction>, TString> {
            const auto& status = fut.GetValueSync();
            if (const auto opRes = operationContext.ReportResult(status); !opRes) {
                return opRes.Error();
            }

            NYdb::TResultSetParser parser(resultSet->GetRef());
            if (parser.TryNextRow()) {
                NMatrix::NApi::TScheduledAction scheduledAction;

                if (auto res = operationContext.ParseProtoFromStringWithErrorReport(*scheduledAction.MutableMeta(), *parser.ColumnParser("meta").GetOptionalString()); !res) {
                    return res.Error();
                }

                if (auto res = operationContext.ParseProtoFromStringWithErrorReport(*scheduledAction.MutableSpec(), *parser.ColumnParser("spec").GetOptionalString()); !res) {
                    return res.Error();
                }

                if (auto res = operationContext.ParseProtoFromStringWithErrorReport(*scheduledAction.MutableStatus(), *parser.ColumnParser("status").GetOptionalString()); !res) {
                    return res.Error();
                }

                return TMaybe<NMatrix::NApi::TScheduledAction>(scheduledAction);
            } else {
                return TMaybe<NMatrix::NApi::TScheduledAction>(Nothing());
            }
        }
    );
}

} // namespace NMatrix::NWorker
