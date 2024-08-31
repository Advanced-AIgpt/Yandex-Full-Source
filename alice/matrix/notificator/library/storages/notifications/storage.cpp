#include "storage.h"

#include <alice/matrix/notificator/library/storages/utils/utils.h>

#include <util/digest/city.h>
#include <util/string/printf.h>


namespace NMatrix::NNotificator {

TNotificationsStorage::TNotificationsStorage(
    const NYdb::TDriver& driver,
    const TYDBClientSettings& config
)
    : IYDBStorage(driver, config, NAME)
{}

NThreading::TFuture<TExpected<TNotificationsStorage::TUserNotificationsState, TString>> TNotificationsStorage::GetUserNotificationsState(
    const TString& puid,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "get_user_notifications_state";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, puid, resultSet](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;

            SELECT ts, last_min_ts
                FROM notification_sync
            WHERE shard_key = $shard_key
                AND yandexid = $yandexid
                AND device_id IS NULL;
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(LegacyComputeShardKey(puid))
                .Build()
            .AddParam("$yandexid")
                .String(puid)
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
    }, RetryOperationSettings_).Apply([resultSet, operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<TUserNotificationsState, TString> {
        const auto& status = fut.GetValueSync();
        if (const auto opRes = operationContext.ReportResult(status); !opRes) {
            return opRes.Error();
        }

        NYdb::TResultSetParser parser(resultSet->GetRef());
        while (parser.TryNextRow()) {
            return TUserNotificationsState({
                .VersionId = *parser.ColumnParser("ts").GetOptionalUint64(),
                .LastTimestamp = parser.ColumnParser("last_min_ts").GetOptionalUint64().GetOrElse(0),
            });
        }

        return TUserNotificationsState({
            .VersionId = 0,
            .LastTimestamp = 0,
        });
    });
}

NThreading::TFuture<TExpected<TNotificationsStorage::TUserNotificationsState, TString>> TNotificationsStorage::UpdateUserNotificationsState(
    const TString& puid,
    const TMaybe<ui64> lastTimestamp,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "update_user_notifications_state";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, puid, lastTimestamp, resultSet](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            --!syntax_v1

            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;
            DECLARE $last_min_ts AS Uint64?;

            $row = (
                SELECT AsTuple(ts, last_min_ts)
                    FROM notification_sync
                WHERE shard_key = $shard_key
                    AND yandexid = $yandexid
                    AND device_id IS NULL
            );

            $ts = IF($row.0 IS NOT NULL, $row.0 + 1, 0);
            $lts = IF($last_min_ts IS NULL, $row.1, $last_min_ts);

            UPSERT INTO notification_sync
                (shard_key, yandexid, device_id, ts, last_min_ts)
            VALUES
                ($shard_key, $yandexid, NULL, $ts, $lts);

            SELECT
                $ts AS ts,
                $lts AS lts
            ;
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(LegacyComputeShardKey(puid))
                .Build()
            .AddParam("$yandexid")
                .String(puid)
                .Build()
            .AddParam("$last_min_ts")
                .OptionalUint64(lastTimestamp)
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
    }, RetryOperationSettings_).Apply([resultSet, operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<TNotificationsStorage::TUserNotificationsState, TString> {
        const auto& status = fut.GetValueSync();
        if (const auto opRes = operationContext.ReportResult(status); !opRes) {
            return opRes.Error();
        }

        NYdb::TResultSetParser parser(resultSet->GetRef());
        if (parser.TryNextRow()) {
            return TNotificationsStorage::TUserNotificationsState({
                .VersionId = *parser.ColumnParser("ts").GetOptionalUint64(),
                .LastTimestamp = parser.ColumnParser("lts").GetOptionalUint64().GetOrElse(0),
            });
        } else {
            // Error (unreachable)
            static const TString err = "Unable to get selected timestamp after state user notifications update";
            return err;
        }
    });
}

NThreading::TFuture<TExpected<TVector<TNotificationsStorage::TNotification>, TString>> TNotificationsStorage::GetNotifications(
    const TString& puid,
    const TMaybe<TString>& deviceId,
    const bool fromArchive,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "get_notifications";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, puid, deviceId, fromArchive, resultSet](NYdb::NTable::TSession session) {
        const auto tableName = fromArchive ? "archived_notifications" : "current_notifications";
        const auto query = Sprintf(R"(
            --!syntax_v1

            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;
            DECLARE $device_id AS String?;
            DECLARE $read AS String;

            SELECT device_id, notification, ts
                FROM %s
            WHERE shard_key = $shard_key
                AND yandexid = $yandexid
                AND (device_id IS NULL OR ($device_id IS NOT NULL AND device_id = $device_id))
                AND (NOT read OR $read = 'any')
            ORDER BY ts DESC
            LIMIT %u;
        )", tableName, NOTIFICATIONS_SELECT_LIMIT);

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(LegacyComputeShardKey(puid))
                .Build()
            .AddParam("$yandexid")
                .String(puid)
                .Build()
            .AddParam("$device_id")
                .OptionalString(deviceId)
                .Build()
            .AddParam("$read")
                .String(fromArchive ? "any" : "read")
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
    }, RetryOperationSettings_).Apply([resultSet, operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<TVector<TNotification>, TString> {
        const auto& status = fut.GetValueSync();
        if (const auto opRes = operationContext.ReportResult(status); !opRes) {
            return opRes.Error();
        }

        NYdb::TResultSetParser parser(resultSet->GetRef());
        TVector<TNotification> result;
        result.reserve(parser.RowsCount());
        while (parser.TryNextRow()) {
            TNotification notification;
            notification.DeviceId = parser.ColumnParser("device_id").GetOptionalString();
            auto protoParseRes = operationContext.ParseProtoFromStringWithErrorReport(notification.Notification, *parser.ColumnParser("notification").GetOptionalString());
            if (protoParseRes.IsError()) {
                return protoParseRes.Error();
            }
            notification.Notification.SetTimestamp(ToString(*parser.ColumnParser("ts").GetOptionalUint64()));
            result.emplace_back(std::move(notification));
        }

        return result;
    });
}

NThreading::TFuture<TExpected<ui64, TString>> TNotificationsStorage::GetArchivedNotificationsCount(
    const TString& puid,
    const TMaybe<TString>& deviceId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "get_archived_notifications_count";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, puid, deviceId, resultSet](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            --!syntax_v1

            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;
            DECLARE $device_id AS String?;

            SELECT COUNT(*) AS count
                FROM archived_notifications
            WHERE shard_key = $shard_key
                AND yandexid = $yandexid
                AND (device_id IS NULL OR ($device_id IS NOT NULL AND device_id = $device_id))
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(LegacyComputeShardKey(puid))
                .Build()
            .AddParam("$yandexid")
                .String(puid)
                .Build()
            .AddParam("$device_id")
                .OptionalString(deviceId)
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
    }, RetryOperationSettings_).Apply([resultSet, operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<ui64, TString> {
        const auto& status = fut.GetValueSync();
        if (const auto opRes = operationContext.ReportResult(status); !opRes) {
            return opRes.Error();
        }

        NYdb::TResultSetParser parser(resultSet->GetRef());
        if (parser.TryNextRow()) {
            return parser.ColumnParser(0).GetUint64();
        } else {
            // Error (unreachable)
            static const TString err = "Unable to select archived notifications count";
            return err;
        }
    });
}

NThreading::TFuture<TExpected<TNotificationsStorage::EAddNotificationResult, TString>> TNotificationsStorage::AddNotification(
    const TString& puid,
    const TNotification& notification,
    const bool allowNotificationDuplicates,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "add_notification";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, puid, notification, allowNotificationDuplicates](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            --!syntax_v1

            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;
            DECLARE $notification_id AS String?;
            DECLARE $notification AS String;
            DECLARE $ts AS Uint64;
            DECLARE $ttl AS Datetime;
            DECLARE $archived_ttl AS Datetime;
            DECLARE $unid AS String;
            DECLARE $device_id AS String?;
            DECLARE $hash AS Uint64;
            DECLARE $allow_notification_duplicates AS Bool;

            DISCARD SELECT Ensure(
                0,
                $allow_notification_duplicates,
                "Notification already exists"
            )
            FROM unique_notifications
            WHERE
                shard_key = $shard_key AND
                yandexid = $yandexid AND
                hash = $hash
            ;

            UPSERT INTO unique_notifications
                (shard_key, yandexid, hash, ttl)
            VALUES
                ($shard_key, $yandexid, $hash, $ttl);

            UPSERT INTO archived_notifications
                (shard_key, yandexid, device_id, unid, notification_id, read, notification, ts, ttl)
            VALUES
                ($shard_key, $yandexid, $device_id, $unid, $notification_id, false, $notification, $ts, $archived_ttl);

            UPSERT INTO current_notifications
                (shard_key, yandexid, device_id, unid, notification_id, read, notification, ts, ttl)
            VALUES
                ($shard_key, $yandexid, $device_id, $unid, $notification_id, false, $notification, $ts, $ttl);
        )";

        // As is from here https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/ydbs/notificator.py?rev=r8625688#L697
        const auto now = TInstant::Now();
        const auto ttlStartTimestamp = TInstant::Days(now.Days()) + TDuration::Days(1) + TDuration::Hours(2);
        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(LegacyComputeShardKey(puid))
                .Build()
            .AddParam("$yandexid")
                .String(puid)
                .Build()
            .AddParam("$notification_id")
                // Magic
                // Do not touch
                .OptionalString("useless")
                .Build()
            .AddParam("$unid")
                .String(notification.Notification.GetId())
                .Build()
            .AddParam("$notification")
                .String(notification.Notification.SerializeAsString())
                .Build()
            .AddParam("$ts")
                .Uint64(now.MicroSeconds())
                .Build()
            .AddParam("$ttl")
                .Datetime(ttlStartTimestamp + CURRENT_NOTIFICATION_TTL)
                .Build()
            .AddParam("$archived_ttl")
                .Datetime(ttlStartTimestamp + ARCHIVED_NOTIFICATION_TTL)
                .Build()
            .AddParam("$device_id")
                .OptionalString(notification.DeviceId)
                .Build()
            .AddParam("$hash")
                .Uint64(notification.GetNotificationContentHash())
                .Build()
            .AddParam("$allow_notification_duplicates")
                .Bool(allowNotificationDuplicates)
                .Build()
            .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            std::move(params),
            ExecDataQuerySettings_
        ).Apply([](const NYdb::NTable::TAsyncDataQueryResult& fut) -> NYdb::TStatus {
            return fut.GetValueSync();
        });
    }, RetryOperationSettings_).Apply([operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<EAddNotificationResult, TString> {
        static IYDBStorage::TResultChecker resultChecker = [](const NYdb::TStatus& res) -> TExpected<void, TString> {
            if (res.IsSuccess()) {
                return TExpected<void, TString>::DefaultSuccess();
            } else {
                const TString error = IYDBStorage::YDBStatusToString(res);

                if (res.GetStatus() == NYdb::EStatus::GENERIC_ERROR && error.find("Notification already exists")) {
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

        // Custom checker handles all errors correctly, so if the operation failed and we are here, it means we have a duplicate
        if (status.IsSuccess()) {
            return EAddNotificationResult::ADDED;
        } else {
            return EAddNotificationResult::ALREADY_EXIST;
        }
    });
}

NThreading::TFuture<TExpected<void, TString>> TNotificationsStorage::MarkNotificationsAsRead(
    const TString& puid,
    const TVector<TString>& notificationIds,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "mark_notifications_as_read";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, puid, notificationIds](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            --!syntax_v1

            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;
            DECLARE $notification_ids AS List<String>;

            UPDATE archived_notifications
               SET read = true
            WHERE
               shard_key = $shard_key AND
               yandexid = $yandexid AND
               unid IN $notification_ids
            ;

            UPDATE current_notifications
               SET read = true
            WHERE
               shard_key = $shard_key AND
               yandexid = $yandexid AND
               unid IN $notification_ids
            ;
        )";

        auto paramsBuilder = session.GetParamsBuilder();

        paramsBuilder
            .AddParam("$shard_key")
                .Uint64(LegacyComputeShardKey(puid))
                .Build()
            .AddParam("$yandexid")
                .String(puid)
                .Build();

        auto& param = paramsBuilder.AddParam("$notification_ids");
        if (notificationIds.empty()) {
            param.EmptyList(NYdb::TTypeBuilder().Primitive(NYdb::EPrimitiveType::String).Build()).Build();
        } else {
            param.BeginList();
            for (const auto& notificationId : notificationIds) {
                param.AddListItem(NYdb::TValueBuilder().String(notificationId).Build());
            }
            param.EndList().Build();
        }

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            paramsBuilder.Build(),
            ExecDataQuerySettings_
        ).Apply([](const NYdb::NTable::TAsyncDataQueryResult& fut) -> NYdb::TStatus {
            return fut.GetValueSync();
        });
    }, RetryOperationSettings_).Apply([operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable {
        return operationContext.ReportResult(fut.GetValueSync());
    });
}

NThreading::TFuture<TExpected<void, TString>> TNotificationsStorage::RemoveAllUserData(
    const TString& puid,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "remove_all_user_data";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, puid](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;

            DELETE FROM unique_notifications
            WHERE
                shard_key = $shard_key AND
                yandexid = $yandexid
            ;

            DELETE FROM current_notifications
            WHERE
                shard_key = $shard_key AND
                yandexid = $yandexid
            ;

            DELETE FROM archived_notifications
            WHERE
                shard_key = $shard_key AND
                yandexid = $yandexid
            ;
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(LegacyComputeShardKey(puid))
                .Build()
            .AddParam("$yandexid")
                .String(puid)
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
    }, RetryOperationSettings_).Apply([operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable {
        return operationContext.ReportResult(fut.GetValueSync());
    });
}

ui64 TNotificationsStorage::TNotification::GetNotificationContentHash() const {
    return MultiHash(
        CityHash64(Notification.GetSubscriptionId()),
        CityHash64(Notification.GetVoice()),
        CityHash64(Notification.GetText())
    );
}

} // namespace NMatrix::NNotificator
