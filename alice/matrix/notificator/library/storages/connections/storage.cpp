#include "storage.h"

#include <util/digest/city.h>
#include <util/string/printf.h>

namespace NMatrix::NNotificator {

TConnectionsStorage::TConnectionsStorage(
    const NYdb::TDriver& driver,
    const TYDBClientSettings& config
)
    : IYDBStorage(driver, config, NAME)
{}

NThreading::TFuture<TExpected<void, TString>> TConnectionsStorage::UpdateConnectionsWithDiff(
    const TConnectionsDiff& connectionsDiff,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    if (connectionsDiff.ConnectedClients.empty() && connectionsDiff.DisconnectedClients.empty()) {
        return NThreading::MakeFuture<TExpected<void, TString>>(TExpected<void, TString>::DefaultSuccess());
    } else if (connectionsDiff.DisconnectedClients.empty()) {
        return UpdateConnectionsWithDiffAddConnected(connectionsDiff.Endpoint, connectionsDiff.ConnectedClients, logContext, metrics);
    } else if (connectionsDiff.ConnectedClients.empty()) {
        return UpdateConnectionsWithDiffRemoveDisconnected(connectionsDiff.Endpoint, connectionsDiff.DisconnectedClients, logContext, metrics);
    } else {
        // Order of operations is important
        // so we need to create execution chain
        return UpdateConnectionsWithDiffRemoveDisconnected(connectionsDiff.Endpoint, connectionsDiff.DisconnectedClients, logContext, metrics).Apply(
            [this, logContext, &metrics, endpoint = connectionsDiff.Endpoint, connectedClients = connectionsDiff.ConnectedClients](
                NThreading::TFuture<TExpected<void, TString>> updateConnectionsWithDiffRemoveDisconnectedResultFut
            ) mutable {
                auto updateConnectionsWithDiffRemoveDisconnectedResult = updateConnectionsWithDiffRemoveDisconnectedResultFut.GetValueSync();
                if (!updateConnectionsWithDiffRemoveDisconnectedResult) {
                    return updateConnectionsWithDiffRemoveDisconnectedResultFut;
                }

                return UpdateConnectionsWithDiffAddConnected(endpoint, connectedClients, logContext, metrics);
            }
        );
    }
}

NThreading::TFuture<TExpected<TConnectionsStorage::TUpdateConnectionsWithFullStateResult, TString>> TConnectionsStorage::UpdateConnectionsWithFullState(
    const TConnectionsFullState& connectionsFullState,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    if (connectionsFullState.ConnectedClients.empty()) {
        return UpdateConnectionsWithFullStateRemoveAll(
            connectionsFullState.Endpoint,
            logContext,
            metrics
        ).Apply(
            [](NThreading::TFuture<TExpected<ui64, TString>> updateConnectionsWithFullStateRemoveAllResultFut) -> TExpected<TConnectionsStorage::TUpdateConnectionsWithFullStateResult, TString> {
                auto updateConnectionsWithFullStateRemoveAllResult = updateConnectionsWithFullStateRemoveAllResultFut.GetValueSync();
                if (!updateConnectionsWithFullStateRemoveAllResult) {
                    return updateConnectionsWithFullStateRemoveAllResult.Error();
                }

                return TUpdateConnectionsWithFullStateResult({
                    .AddedCount = 0,
                    .RemovedCount = updateConnectionsWithFullStateRemoveAllResult.Success(),

                    .AddedClients = {},
                    .IsAddedClientsTruncated = false,
                });
            }
        );
    } else {
        return UpdateConnectionsWithFullStateRemoveDisconnected(connectionsFullState, logContext, metrics).Apply(
            [this, logContext, &metrics, connectionsFullState](
                NThreading::TFuture<TExpected<ui64, TString>> updateConnectionsWithFullStateRemoveDisconnectedResultFut
            ) mutable {
                auto updateConnectionsWithFullStateRemoveDisconnectedResult = updateConnectionsWithFullStateRemoveDisconnectedResultFut.GetValueSync();
                if (!updateConnectionsWithFullStateRemoveDisconnectedResult) {
                    return NThreading::MakeFuture<TExpected<TConnectionsStorage::TUpdateConnectionsWithFullStateResult, TString>>(updateConnectionsWithFullStateRemoveDisconnectedResult.Error());
                }

                return UpdateConnectionsWithFullStateAddConnected(connectionsFullState, logContext, metrics).Apply(
                    [removedCount = updateConnectionsWithFullStateRemoveDisconnectedResult.Success()] (
                        NThreading::TFuture<TExpected<TUpdateConnectionsWithFullStateAddConnectedResult, TString>> updateConnectionsWithFullStateAddConnectedResultFut
                    ) -> TExpected<TConnectionsStorage::TUpdateConnectionsWithFullStateResult, TString> {
                        auto updateConnectionsWithFullStateAddConnectedResult = updateConnectionsWithFullStateAddConnectedResultFut.GetValueSync();
                        if (!updateConnectionsWithFullStateAddConnectedResult) {
                            return updateConnectionsWithFullStateAddConnectedResult.Error();
                        }
                        const auto updateConnectionsWithFullStateAddConnected = updateConnectionsWithFullStateAddConnectedResult.Success();

                        return TUpdateConnectionsWithFullStateResult({
                            .AddedCount = updateConnectionsWithFullStateAddConnected.AddedCount,
                            .RemovedCount = removedCount,

                            .AddedClients = updateConnectionsWithFullStateAddConnected.AddedClients,
                            .IsAddedClientsTruncated = updateConnectionsWithFullStateAddConnected.IsAddedClientsTruncated,
                        });
                    }
                );
            }
        );
    }
}

NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>> TConnectionsStorage::ListConnections(
    const TString& puid,
    const TMaybe<TString>& deviceId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    if (deviceId.Defined()) {
        return ListConnectionsByPuidAndDeviceId(puid, *deviceId, logContext, metrics);
    } else {
        return ListConnectionsByPuid(puid, logContext, metrics);
    }
}

NThreading::TFuture<TExpected<void, TString>> TConnectionsStorage::UpdateConnectionsWithDiffRemoveDisconnected(
    const TEndpoint& endpoint,
    const TVector<TUserDeviceInfo>& disconnectedClients,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "update_connections_with_diff_remove_disconnected";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, endpoint, disconnectedClients](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            --!syntax_v1

            PRAGMA AnsiInForEmptyOrNullableItemsCollections;

            DECLARE $shard_key AS Uint64;
            DECLARE $ip AS String;
            DECLARE $ip_shard_id AS Uint64;

            DECLARE $disconnected_clients AS List<
                Tuple<
                    String,
                    String,
                >
            >;

            DELETE FROM connections_sharded WHERE
                shard_key = $shard_key AND
                ip = $ip AND
                ip_shard_id = $ip_shard_id AND
                (puid, device_id) IN COMPACT $disconnected_clients
            ;
        )";

        auto paramsBuilder = session.GetParamsBuilder();
        auto shardKey = endpoint.GetTableShardKey();

        paramsBuilder
            .AddParam("$shard_key")
                .Uint64(shardKey)
                .Build()
            .AddParam("$ip")
                .String(endpoint.Ip)
                .Build()
            .AddParam("$ip_shard_id")
                .Uint64(endpoint.ShardId)
                .Build();

        {
            auto& param = paramsBuilder.AddParam("$disconnected_clients");
            param.BeginList();
            for (const auto& disconnectedClient : disconnectedClients) {
                param.AddListItem()
                    .BeginTuple()
                        .AddElement().String(disconnectedClient.Puid)
                        .AddElement().String(disconnectedClient.DeviceId)
                    .EndTuple();
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

NThreading::TFuture<TExpected<void, TString>> TConnectionsStorage::UpdateConnectionsWithDiffAddConnected(
    const TEndpoint& endpoint,
    const TVector<TUserDeviceInfo>& connectedClients,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "update_connections_with_diff_add_connected";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, endpoint, connectedClients](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            --!syntax_v1

            DECLARE $shard_key AS Uint64;
            DECLARE $ip AS String;
            DECLARE $ip_shard_id AS Uint64;
            DECLARE $port AS Uint32;
            DECLARE $monotonic AS Uint64;
            DECLARE $created_at AS Timestamp;
            DECLARE $expired_at AS Timestamp;

            DECLARE $connected_clients AS List<
                Struct<
                    puid: String,
                    device_id: String,
                    device_info: String,
                >
            >;

            UPSERT INTO connections_sharded (
                SELECT
                    $shard_key AS shard_key,
                    $ip AS ip,
                    $ip_shard_id AS ip_shard_id,
                    puid AS puid,
                    device_id as device_id,
                    $port AS port,
                    $monotonic AS monotonic,
                    device_info AS device_info,
                    $created_at AS created_at,
                    $expired_at AS expired_at,
                FROM AS_TABLE($connected_clients)
            );
        )";

        auto paramsBuilder = session.GetParamsBuilder();
        auto shardKey = endpoint.GetTableShardKey();
        TInstant createdAt = TInstant::Now();
        TInstant expiredAt = createdAt + CONNECTION_TTL;

        paramsBuilder
            .AddParam("$shard_key")
                .Uint64(shardKey)
                .Build()
            .AddParam("$ip")
                .String(endpoint.Ip)
                .Build()
            .AddParam("$ip_shard_id")
                .Uint64(endpoint.ShardId)
                .Build()
            .AddParam("$port")
                .Uint32(endpoint.Port)
                .Build()
            .AddParam("$monotonic")
                .Uint64(endpoint.Monotonic)
                .Build()
            .AddParam("$created_at")
                .Timestamp(createdAt)
                .Build()
            .AddParam("$expired_at")
                .Timestamp(expiredAt)
                .Build();

        {
            auto& param = paramsBuilder.AddParam("$connected_clients");

            param.BeginList();
            for (const auto& connectedClient : connectedClients) {
                param.AddListItem()
                    .BeginStruct()
                        .AddMember("puid").String(connectedClient.Puid)
                        .AddMember("device_id").String(connectedClient.DeviceId)
                        .AddMember("device_info").String(connectedClient.DeviceInfo.SerializeAsString())
                    .EndStruct();
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


NThreading::TFuture<TExpected<ui64, TString>> TConnectionsStorage::UpdateConnectionsWithFullStateRemoveAll(
    const TEndpoint& endpoint,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "update_connections_with_full_state_remove_all";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, resultSet, endpoint](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            --!syntax_v1

            DECLARE $shard_key AS Uint64;
            DECLARE $ip AS String;
            DECLARE $ip_shard_id AS Uint64;

            $to_delete = (
                SELECT * FROM connections_sharded WHERE
                    shard_key = $shard_key AND
                    ip = $ip AND
                    ip_shard_id = $ip_shard_id
            );

            SELECT COUNT(*) AS removed FROM $to_delete;

            DELETE FROM connections_sharded ON
            SELECT * FROM $to_delete;
        )";

        auto paramsBuilder = session.GetParamsBuilder();
        auto shardKey = endpoint.GetTableShardKey();

        paramsBuilder
            .AddParam("$shard_key")
                .Uint64(shardKey)
                .Build()
            .AddParam("$ip")
                .String(endpoint.Ip)
                .Build()
            .AddParam("$ip_shard_id")
                .Uint64(endpoint.ShardId)
                .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            paramsBuilder.Build(),
            ExecDataQuerySettings_
        ).Apply([resultSet](const NYdb::NTable::TAsyncDataQueryResult& fut) -> NYdb::TStatus {
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
            return parser.ColumnParser("removed").GetUint64();
        } else {
            operationContext.Metrics.PushRate(TString::Join(operationContext.FullOperationName, "_missed_result"), "error", "ydb");
            return 0;
        }
    });
}

NThreading::TFuture<TExpected<ui64, TString>> TConnectionsStorage::UpdateConnectionsWithFullStateRemoveDisconnected(
    const TConnectionsFullState& connectionsFullState,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "update_connections_with_full_state_remove_disconnected";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, resultSet, connectionsFullState](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            --!syntax_v1

            PRAGMA AnsiInForEmptyOrNullableItemsCollections;

            DECLARE $shard_key AS Uint64;
            DECLARE $ip AS String;
            DECLARE $ip_shard_id AS Uint64;

            DECLARE $connected_clients AS List<
                Tuple<
                    String,
                    String,
                >
            >;

            $to_delete = (
                SELECT * FROM connections_sharded WHERE
                    shard_key = $shard_key AND
                    ip = $ip AND
                    ip_shard_id = $ip_shard_id AND
                    (puid, device_id) NOT IN COMPACT $connected_clients
            );

            SELECT COUNT(*) AS removed FROM $to_delete;

            DELETE FROM connections_sharded ON
            SELECT * FROM $to_delete;
        )";

        auto paramsBuilder = session.GetParamsBuilder();
        auto shardKey = connectionsFullState.Endpoint.GetTableShardKey();

        paramsBuilder
            .AddParam("$shard_key")
                .Uint64(shardKey)
                .Build()
            .AddParam("$ip")
                .String(connectionsFullState.Endpoint.Ip)
                .Build()
            .AddParam("$ip_shard_id")
                .Uint64(connectionsFullState.Endpoint.ShardId)
                .Build();

        {
            auto& param = paramsBuilder.AddParam("$connected_clients");
            param.BeginList();
            for (const auto& connectedClient : connectionsFullState.ConnectedClients) {
                param.AddListItem()
                    .BeginTuple()
                        .AddElement().String(connectedClient.Puid)
                        .AddElement().String(connectedClient.DeviceId)
                    .EndTuple();
            }
            param.EndList().Build();
        }

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            paramsBuilder.Build(),
            ExecDataQuerySettings_
        ).Apply([resultSet](const NYdb::NTable::TAsyncDataQueryResult& fut) -> NYdb::TStatus {
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
            return parser.ColumnParser("removed").GetUint64();
        } else {
            operationContext.Metrics.PushRate(TString::Join(operationContext.FullOperationName, "_missed_result"), "error", "ydb");
            return 0;
        }
    });
}

NThreading::TFuture<TExpected<TConnectionsStorage::TUpdateConnectionsWithFullStateAddConnectedResult, TString>> TConnectionsStorage::UpdateConnectionsWithFullStateAddConnected(
    const TConnectionsFullState& connectionsFullState,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "update_connections_with_full_state_add_connected";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultAddedCount = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    auto resultAddedClients = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, resultAddedCount, resultAddedClients, connectionsFullState](NYdb::NTable::TSession session) {
        static const auto query = Sprintf(R"(
            --!syntax_v1

            PRAGMA AnsiInForEmptyOrNullableItemsCollections;

            DECLARE $shard_key AS Uint64;
            DECLARE $ip AS String;
            DECLARE $ip_shard_id AS Uint64;
            DECLARE $port AS Uint32;
            DECLARE $monotonic AS Uint64;
            DECLARE $created_at AS Timestamp;
            DECLARE $expired_at AS Timestamp;

            DECLARE $connected_clients AS List<
                Struct<
                    puid: String,
                    device_id: String,
                    device_info: String,
                >
            >;

            $connected_clients_from_database = (
                SELECT (puid, device_id) FROM connections_sharded
                WHERE
                    shard_key = $shard_key AND
                    ip = $ip AND
                    ip_shard_id = $ip_shard_id
            );

            $connected_clients_to_add = (
                SELECT puid, device_id, device_info FROM AS_TABLE($connected_clients)
                WHERE (puid, device_id) NOT IN COMPACT $connected_clients_from_database
            );

            SELECT COUNT(*) AS added FROM $connected_clients_to_add;

            SELECT puid, device_id, device_info FROM $connected_clients_to_add
            LIMIT %u;

            UPSERT INTO connections_sharded (
                SELECT
                    $shard_key AS shard_key,
                    $ip AS ip,
                    $ip_shard_id as ip_shard_id,
                    puid AS puid,
                    device_id AS device_id,
                    $port AS port,
                    $monotonic AS monotonic,
                    device_info AS device_info,
                    $created_at AS created_at,
                    $expired_at AS expired_at
                FROM $connected_clients_to_add
            );
        )", MAX_ADDED_USER_DEVICES_IN_FULL_STATE_UPDATE_RESULT);

        auto paramsBuilder = session.GetParamsBuilder();
        auto shardKey = connectionsFullState.Endpoint.GetTableShardKey();
        TInstant createdAt = TInstant::Now();
        TInstant expiredAt = createdAt + CONNECTION_TTL;

        paramsBuilder
            .AddParam("$shard_key")
                .Uint64(shardKey)
                .Build()
            .AddParam("$ip")
                .String(connectionsFullState.Endpoint.Ip)
                .Build()
            .AddParam("$ip_shard_id")
                .Uint64(connectionsFullState.Endpoint.ShardId)
                .Build()
            .AddParam("$port")
                .Uint32(connectionsFullState.Endpoint.Port)
                .Build()
            .AddParam("$monotonic")
                .Uint64(connectionsFullState.Endpoint.Monotonic)
                .Build()
            .AddParam("$created_at")
                .Timestamp(createdAt)
                .Build()
            .AddParam("$expired_at")
                .Timestamp(expiredAt)
                .Build();

        {
            auto& param = paramsBuilder.AddParam("$connected_clients");

            param.BeginList();
            for (const auto& connectedClient : connectionsFullState.ConnectedClients) {
                param.AddListItem()
                    .BeginStruct()
                        .AddMember("puid").String(connectedClient.Puid)
                        .AddMember("device_id").String(connectedClient.DeviceId)
                        .AddMember("device_info").String(connectedClient.DeviceInfo.SerializeAsString())
                    .EndStruct();
            }
            param.EndList().Build();
        }

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            paramsBuilder.Build(),
            ExecDataQuerySettings_
        ).Apply([resultAddedCount, resultAddedClients](const NYdb::NTable::TAsyncDataQueryResult& fut) -> NYdb::TStatus {
            const auto& res = fut.GetValueSync();
            if (res.IsSuccess()) {
                *resultAddedCount = res.GetResultSet(0);
                *resultAddedClients = res.GetResultSet(1);
            }

            return res;
        });
    }, RetryOperationSettings_).Apply(
        [resultAddedCount, resultAddedClients, operationContext = std::move(operationContext)](
            const NYdb::TAsyncStatus& fut
        ) mutable -> TExpected<TUpdateConnectionsWithFullStateAddConnectedResult, TString> {
            const auto& status = fut.GetValueSync();
            if (const auto opRes = operationContext.ReportResult(status); !opRes) {
                return opRes.Error();
            }

            TUpdateConnectionsWithFullStateAddConnectedResult result;

            {
                NYdb::TResultSetParser parser(resultAddedCount->GetRef());
                if (parser.TryNextRow()) {
                    result.AddedCount = parser.ColumnParser("added").GetUint64();
                } else {
                    operationContext.Metrics.PushRate(TString::Join(operationContext.FullOperationName, "_missed_added_count_result"), "error", "ydb");
                    result.AddedCount = 0;
                }
            }

            {
                NYdb::TResultSetParser parser(resultAddedClients->GetRef());
                result.AddedClients.reserve(parser.RowsCount());
                while (parser.TryNextRow()) {
                    TUserDeviceInfo userDeviceInfo;

                    // Parse device_info first for fast error report
                    const TString rawDeviceInfo = parser.ColumnParser("device_info").GetString();
                    auto protoParseRes = operationContext.ParseProtoFromStringWithErrorReport(userDeviceInfo.DeviceInfo, rawDeviceInfo);
                    if (protoParseRes.IsError()) {
                        return protoParseRes.Error();
                    }

                    userDeviceInfo.Puid = parser.ColumnParser("puid").GetString();
                    userDeviceInfo.DeviceId = parser.ColumnParser("device_id").GetString();

                    result.AddedClients.emplace_back(std::move(userDeviceInfo));
                }
            }

            result.IsAddedClientsTruncated = (result.AddedCount != result.AddedClients.size());
            return result;
        }
    );
}

NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>> TConnectionsStorage::ListConnectionsByPuid(
    const TString& puid,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "list_connections_by_puid";
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
            --!syntax_v1

            DECLARE $puid AS String;

            SELECT puid, device_id, ip, ip_shard_id, port, monotonic, device_info, created_at
            FROM connections_sharded view ix_puid_device_id_async
            WHERE puid = $puid;
        )";

        auto paramsBuilder = session.GetParamsBuilder();
        paramsBuilder
            .AddParam("$puid")
                .String(puid)
                .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::StaleRO()).CommitTx(),
            paramsBuilder.Build(),
            ExecDataQuerySettings_
        ).Apply([resultSet](const NYdb::NTable::TAsyncDataQueryResult& fut) mutable -> NYdb::TStatus {
            const auto& res = fut.GetValueSync();
            if (res.IsSuccess()) {
                *resultSet = res.GetResultSet(0);
            }

            return res;
        });
    }, RetryOperationSettings_).Apply(
        [resultSet, operationContext = std::move(operationContext)](
            const NYdb::TAsyncStatus& fut
        ) mutable -> TExpected<TConnectionsStorage::TListConnectionsResult, TString> {

            const auto& status = fut.GetValueSync();
            if (const auto opRes = operationContext.ReportResult(status); !opRes) {
                return opRes.Error();
            }

            return ParseListConnectionsResult(
                resultSet->GetRef(),
                operationContext
            );
        }
    );
}

NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>> TConnectionsStorage::ListConnectionsByPuidAndDeviceId(
    const TString& puid,
    const TString& deviceId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "list_connections_by_puid_and_device_id";
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

            DECLARE $puid AS String;
            DECLARE $device_id AS String;

            SELECT puid, device_id, ip, ip_shard_id, port, monotonic, device_info, created_at
            FROM connections_sharded view ix_puid_device_id_async
            WHERE puid = $puid AND device_id = $device_id;
        )";

        auto paramsBuilder = session.GetParamsBuilder();
        paramsBuilder
            .AddParam("$puid")
                .String(puid)
                .Build()
            .AddParam("$device_id")
                .String(deviceId)
                .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::StaleRO()).CommitTx(),
            paramsBuilder.Build(),
            ExecDataQuerySettings_
        ).Apply([resultSet](const NYdb::NTable::TAsyncDataQueryResult& fut) mutable -> NYdb::TStatus {
            const auto& res = fut.GetValueSync();
            if (res.IsSuccess()) {
                *resultSet = res.GetResultSet(0);
            }

            return res;
        });
    }, RetryOperationSettings_).Apply(
        [resultSet, operationContext = std::move(operationContext)](
            const NYdb::TAsyncStatus& fut
        ) mutable -> TExpected<TConnectionsStorage::TListConnectionsResult, TString> {

            const auto& status = fut.GetValueSync();
            if (const auto opRes = operationContext.ReportResult(status); !opRes) {
                return opRes.Error();
            }

            return ParseListConnectionsResult(
                resultSet->GetRef(),
                operationContext
            );
        }
    );
}

TExpected<TConnectionsStorage::TListConnectionsResult, TString> TConnectionsStorage::ParseListConnectionsResult(
    const NYdb::TResultSet& resultSet,
    TOperationContext& operationContext
) {
    NYdb::TResultSetParser parser(resultSet);
    // DeviceId -> <CreatedAt, TConnectionsStorage::TListConnectionsResult::TRecord>
    TMap<TString, std::pair<TInstant, TConnectionsStorage::TListConnectionsResult::TRecord>> deviceIdToConnectionRecord;
    while (parser.TryNextRow()) {
        TConnectionsStorage::TListConnectionsResult::TRecord connectionRecord = {
            .Endpoint = TConnectionsStorage::TEndpoint({
                .Ip = *parser.ColumnParser("ip").GetOptionalString(),
                .ShardId = *parser.ColumnParser("ip_shard_id").GetOptionalUint64(),
                .Port = *parser.ColumnParser("port").GetOptionalUint32(),
                .Monotonic = *parser.ColumnParser("monotonic").GetOptionalUint64(),
            }),
            .UserDeviceInfo = TConnectionsStorage::TUserDeviceInfo({
                .Puid = *parser.ColumnParser("puid").GetOptionalString(),
                .DeviceId =  *parser.ColumnParser("device_id").GetOptionalString(),
                .DeviceInfo = {}
            }),
        };

        const TString rawDeviceInfo = *parser.ColumnParser("device_info").GetOptionalString();
        auto protoParseRes = operationContext.ParseProtoFromStringWithErrorReport(connectionRecord.UserDeviceInfo.DeviceInfo, rawDeviceInfo);
        if (protoParseRes.IsError()) {
            return protoParseRes.Error();
        }

        const auto createdAt = *parser.ColumnParser("created_at").GetOptionalTimestamp();
        if (auto [iter, inserted] = deviceIdToConnectionRecord.insert({connectionRecord.UserDeviceInfo.DeviceId, {createdAt, connectionRecord}}); !inserted && iter->second.first < createdAt) {
            iter->second = {createdAt, std::move(connectionRecord)};
        }
    }

    TConnectionsStorage::TListConnectionsResult result;
    result.Records.reserve(deviceIdToConnectionRecord.size());
    for (auto& [deviceId, deviceInfo] : deviceIdToConnectionRecord) {
        auto& [createdAt, connectionRecord] = deviceInfo;
        result.Records.emplace_back(std::move(connectionRecord));
    }

    return result;
}

ui64 TConnectionsStorage::TEndpoint::GetTableShardKey() const {
    return MultiHash(CityHash64(Ip), ShardId);
}

} // namespace NMatrix::NNotificator
