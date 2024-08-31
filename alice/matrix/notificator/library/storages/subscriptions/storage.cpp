#include "storage.h"

#include <alice/matrix/notificator/library/storages/utils/utils.h>


namespace NMatrix::NNotificator {

TSubscriptionsStorage::TSubscriptionsStorage(
    const NYdb::TDriver& driver,
    const TYDBClientSettings& config
)
    : IYDBStorage(driver, config, NAME)
{}

NThreading::TFuture<TExpected<TMap<ui64, TSubscriptionsStorage::TUserSubscription>, TString>> TSubscriptionsStorage::GetUserSubscriptionsForUser(
    const TString& puid,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "get_user_subscriptions_for_user";
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

            SELECT
                subscription_id,
                `timestamp`
            FROM user_subscriptions
            WHERE
                shard_key = $shard_key
                AND yandexid = $yandexid;
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
    }, RetryOperationSettings_).Apply([resultSet, operationContext = std::move(operationContext), puid](const NYdb::TAsyncStatus& fut) mutable -> TExpected<TMap<ui64, TUserSubscription>, TString> {
        const auto& status = fut.GetValueSync();
        if (const auto opRes = operationContext.ReportResult(status); !opRes) {
            return opRes.Error();
        }

        NYdb::TResultSetParser parser(resultSet->GetRef());
        TMap<ui64, TUserSubscription> result;
        while (parser.TryNextRow()) {
            result.insert(
                {
                    *parser.ColumnParser("subscription_id").GetOptionalUint64(),
                    TUserSubscription({
                        .Puid = puid,
                        .SubscribedAtTimestamp = *parser.ColumnParser("timestamp").GetOptionalUint64(),
                    })
                }
            );
        }

        return result;
    });
}

NThreading::TFuture<TExpected<TVector<TSubscriptionsStorage::TUserSubscription>, TString>> TSubscriptionsStorage::GetUserSubscriptionsBySubscriptionId(
    const ui64 subscriptionId,
    const TMaybe<ui64> afterTimestamp,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "get_user_subscriptions_by_subscription_id";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, subscriptionId, afterTimestamp, resultSet](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            --!syntax_v1

            DECLARE $subscription_id as Uint64;
            DECLARE $after_timestamp AS Uint64?;

            SELECT
                subscription_id,
                yandexid,
                `timestamp`
            FROM user_subscriptions VIEW user_subs_list
            WHERE
                subscription_id = $subscription_id AND
                ($after_timestamp IS NULL OR `timestamp` > $after_timestamp)
            ORDER BY subscription_id, `timestamp`
            LIMIT 1000;
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$subscription_id")
                .Uint64(subscriptionId)
                .Build()
            .AddParam("$after_timestamp")
                .OptionalUint64(afterTimestamp)
                .Build()
            .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::StaleRO()).CommitTx(),
            std::move(params),
            ExecDataQuerySettings_
        ).Apply([resultSet](const NYdb::NTable::TAsyncDataQueryResult& fut) mutable -> NYdb::TStatus {
            const auto& res = fut.GetValueSync();
            if (res.IsSuccess()) {
                *resultSet = res.GetResultSet(0);
            }

            return res;
        });
    }, RetryOperationSettings_).Apply([resultSet, operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<TVector<TUserSubscription>, TString> {
        const auto& status = fut.GetValueSync();
        if (const auto opRes = operationContext.ReportResult(status); !opRes) {
            return opRes.Error();
        }

        NYdb::TResultSetParser parser(resultSet->GetRef());
        TVector<TUserSubscription> result;
        result.reserve(parser.RowsCount());
        while (parser.TryNextRow()) {
            result.emplace_back(TUserSubscription({
                .Puid = *parser.ColumnParser("yandexid").GetOptionalString(),
                .SubscribedAtTimestamp = *parser.ColumnParser("timestamp").GetOptionalUint64(),
            }));
        }

        return result;
    });
}

NThreading::TFuture<TExpected<TVector<TString>, TString>> TSubscriptionsStorage::GetUserUnsubscribedDevices(
    const TString& puid,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "get_user_unsubscribed_devices";
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

            SELECT device_id
                FROM device_subscriptions
            WHERE shard_key = $shard_key
                AND yandexid = $yandexid
                AND subscribed = false;
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
    }, RetryOperationSettings_).Apply([resultSet, operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<TVector<TString>, TString> {
        const auto& status = fut.GetValueSync();
        if (const auto opRes = operationContext.ReportResult(status); !opRes) {
            return opRes.Error();
        }

        NYdb::TResultSetParser parser(resultSet->GetRef());
        TVector<TString> result;
        result.reserve(parser.RowsCount());
        while (parser.TryNextRow()) {
            result.emplace_back(*parser.ColumnParser("device_id").GetOptionalString());
        }

        return result;
    });
}

NThreading::TFuture<TExpected<void, TString>> TSubscriptionsStorage::SubscribeUser(
    const TString& puid,
    const ui64 subscriptionId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "subscribe_user";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, puid, subscriptionId](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;
            DECLARE $subscription_id AS Uint64;
            DECLARE $timestamp AS Uint64;

            UPSERT INTO user_subscriptions
                (shard_key, yandexid, device_id, subscription_id, `timestamp`)
            VALUES
                ($shard_key, $yandexid, NULL, $subscription_id, $timestamp);
        )";

        const auto now = TInstant::Now();
        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(LegacyComputeShardKey(puid))
                .Build()
            .AddParam("$yandexid")
                .String(puid)
                .Build()
            .AddParam("$subscription_id")
                .Uint64(subscriptionId)
                .Build()
            .AddParam("$timestamp")
                .Uint64(now.MicroSeconds())
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

NThreading::TFuture<TExpected<void, TString>> TSubscriptionsStorage::UnsubscribeUser(
    const TString& puid,
    const ui64 subscriptionId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "unsubscribe_user";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, puid, subscriptionId](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;
            DECLARE $subscription_id AS Uint64;

            DELETE FROM user_subscriptions
            WHERE
                shard_key = $shard_key AND
                yandexid = $yandexid AND
                device_id IS NULL AND
                subscription_id = $subscription_id
            ;
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(LegacyComputeShardKey(puid))
                .Build()
            .AddParam("$yandexid")
                .String(puid)
                .Build()
            .AddParam("$subscription_id")
                .Uint64(subscriptionId)
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

NThreading::TFuture<TExpected<void, TString>> TSubscriptionsStorage::SubscribeUserDevice(
    const TString& puid,
    const TString& deviceId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "subscribe_user_device";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, puid, deviceId](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;
            DECLARE $device_id AS String;

            DELETE FROM device_subscriptions
            WHERE
                shard_key = $shard_key AND
                yandexid = $yandexid AND
                device_id = $device_id
            ;
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(LegacyComputeShardKey(puid))
                .Build()
            .AddParam("$yandexid")
                .String(puid)
                .Build()
            .AddParam("$device_id")
                .String(deviceId)
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

NThreading::TFuture<TExpected<void, TString>> TSubscriptionsStorage::UnsubscribeUserDevice(
    const TString& puid,
    const TString& deviceId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "unsubscribe_user_device";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, puid, deviceId](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;
            DECLARE $device_id AS String;

            UPSERT INTO device_subscriptions
                (shard_key, yandexid, device_id, subscribed)
            VALUES
                ($shard_key, $yandexid, $device_id, false);
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(LegacyComputeShardKey(puid))
                .Build()
            .AddParam("$yandexid")
                .String(puid)
                .Build()
            .AddParam("$device_id")
                .String(deviceId)
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

NThreading::TFuture<TExpected<void, TString>> TSubscriptionsStorage::RemoveAllUserData(
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

            DELETE FROM user_subscriptions
            WHERE
                shard_key = $shard_key AND
                yandexid = $yandexid
            ;

            DELETE FROM device_subscriptions
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

} // namespace NMatrix::NNotificator
