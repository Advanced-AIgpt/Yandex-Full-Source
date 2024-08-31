#include "storage.h"

#include <alice/matrix/notificator/library/storages/utils/utils.h>

#include <util/digest/city.h>


namespace NMatrix::NNotificator {

TLocatorStorage::TLocatorStorage(
    const NYdb::TDriver& driver,
    const TYDBClientSettings& config
)
    : IYDBStorage(driver, config, NAME)
{}

NThreading::TFuture<TExpected<void, TString>> TLocatorStorage::Store(
    const ::NNotificator::TDeviceLocator& msg,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "store";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, msg](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_key AS Uint64;
            DECLARE $puid AS String;
            DECLARE $device_id AS String;
            DECLARE $host AS String;
            DECLARE $ts AS Uint64;
            DECLARE $device_model AS String;
            DECLARE $created_at AS Datetime;
            DECLARE $config AS String;

            UPSERT INTO device_locator
            (shard_key, puid, host, ts, device_id, device_model, ttl, config)
            VALUES
            ($shard_key, $puid, $host, $ts, $device_id, $device_model, $created_at, $config);
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(CityHash64(msg.GetPuid()))
                .Build()
            .AddParam("$puid")
                .String(msg.GetPuid())
                .Build()
            .AddParam("$device_id")
                .String(msg.GetDeviceId())
                .Build()
            .AddParam("$host")
                .String(msg.GetHost())
                .Build()
            .AddParam("$ts")
                .Uint64(msg.GetTimestamp())
                .Build()
            .AddParam("$device_model")
                .String(msg.GetDeviceModel())
                .Build()
            .AddParam("$created_at")
                .Datetime(TInstant::Now())
                .Build()
            .AddParam("$config")
                .String(msg.GetConfig().SerializeAsString())
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
    }, RetryOperationSettings_).Apply([operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable {
        return operationContext.ReportResult(fut.GetValueSync());
    });
}

NThreading::TFuture<TExpected<void, TString>> TLocatorStorage::Remove(
    const ::NNotificator::TDeviceLocator& msg,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "remove";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, msg](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            --!syntax_v1
            DECLARE $device_id AS String;
            DECLARE $host AS String;
            DECLARE $ts AS Uint64;

            $to_delete = (
                SELECT * FROM device_locator VIEW device_locator_list
                WHERE device_id = $device_id AND host = $host AND ts < $ts
            );

            DELETE FROM device_locator ON
            SELECT * FROM $to_delete;
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$device_id")
                .String(msg.GetDeviceId())
                .Build()
            .AddParam("$host")
                .String(msg.GetHost())
                .Build()
            .AddParam("$ts")
                .Uint64(msg.GetTimestamp())
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
    }, RetryOperationSettings_).Apply([operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable {
        return operationContext.ReportResult(fut.GetValueSync());
    });
}

NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>> TLocatorStorage::List(
    const TString& puid,
    const TString& deviceId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "list";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, puid, deviceId, resultSet](NYdb::NTable::TSession session) {
        static constexpr auto queryWithDeviceId = R"(
            DECLARE $shard_key AS Uint64;
            DECLARE $puid AS String;
            DECLARE $device_id AS String;

            SELECT device_id, host, device_model, config, ts
            FROM device_locator
            WHERE shard_key = $shard_key AND puid = $puid AND device_id = $device_id;
        )";
        static constexpr auto queryWithoutDeviceId = R"(
            DECLARE $shard_key AS Uint64;
            DECLARE $puid AS String;
            DECLARE $device_id AS String;

            SELECT device_id, host, device_model, config, ts
            FROM device_locator
            WHERE shard_key = $shard_key AND puid = $puid;
        )";
        const auto& query = deviceId.empty() ? queryWithoutDeviceId : queryWithDeviceId;

        auto paramsBuilder = session.GetParamsBuilder();
        paramsBuilder
            .AddParam("$shard_key")
                .Uint64(CityHash64(puid))
                .Build()
            .AddParam("$puid")
                .String(puid)
                .Build();

        if (!deviceId.empty()) {
            paramsBuilder.AddParam("$device_id").String(deviceId).Build();
        }

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::OnlineRO()).CommitTx(),
            paramsBuilder.Build(),
            ExecDataQuerySettings_
        ).Apply([resultSet](const NYdb::NTable::TAsyncDataQueryResult& fut) mutable -> NYdb::TStatus {
            const auto& res = fut.GetValueSync();
            if (res.IsSuccess()) {
                *resultSet = res.GetResultSet(0);
            }

            return res;
        });
    }, RetryOperationSettings_).Apply([resultSet, puid, operationContext = std::move(operationContext)](
        const NYdb::TAsyncStatus& fut
    ) mutable -> TExpected<TConnectionsStorage::TListConnectionsResult, TString> {

        const auto& status = fut.GetValueSync();
        if (const auto opRes = operationContext.ReportResult(status); !opRes) {
            return opRes.Error();
        }

        NYdb::TResultSetParser parser(resultSet->GetRef());
        // DeviceId -> <ts, TConnectionsStorage::TListConnectionsResult::TRecord>
        TMap<TString, std::pair<ui64, TConnectionsStorage::TListConnectionsResult::TRecord>> deviceIdToConnectionRecord;
        while (parser.TryNextRow()) {
            // Parse device config first for fast error report
            ::NNotificator::TDeviceConfig deviceConfig;
            if (const auto rawDeviceConfig = parser.ColumnParser("config").GetOptionalString()) {
                auto protoParseRes = operationContext.ParseProtoFromStringWithErrorReport(deviceConfig, *rawDeviceConfig);
                if (protoParseRes.IsError()) {
                    return protoParseRes.Error();
                }
            }

            TConnectionsStorage::TListConnectionsResult::TRecord connectionRecord = {
                .Endpoint = TConnectionsStorage::TEndpoint({
                    // Locator storage uses fqdns, not ips :(
                    // SubwayClient works correctly with fqdn in ip field
                    .Ip = *parser.ColumnParser("host").GetOptionalString(),
                    // Fake shard id in locator storage
                    .ShardId = 0,
                    // No port in locator storage
                    // We assume that default subway port is used
                    .Port = SUBWAY_PORT,
                    // Fake monotonic in locator storage
                    .Monotonic = 0,
                }),
                .UserDeviceInfo = TConnectionsStorage::TUserDeviceInfo({
                    .Puid = puid,
                    .DeviceId =  *parser.ColumnParser("device_id").GetOptionalString(),
                    .DeviceInfo = {}
                }),
            };

            connectionRecord.UserDeviceInfo.DeviceInfo.SetDeviceModel(*parser.ColumnParser("device_model").GetOptionalString());
            for (const auto& supportedFeature : deviceConfig.GetSupportedFeatures()) {
                if (const auto parsedSupportedFeature = TryParseSupportedFeatureFromString(supportedFeature)) {
                    connectionRecord.UserDeviceInfo.DeviceInfo.AddSupportedFeatures(*parsedSupportedFeature);
                }
            }

            const auto ts = parser.ColumnParser("ts").GetOptionalUint64().GetOrElse(0);

            if (auto [iter, inserted] = deviceIdToConnectionRecord.insert({connectionRecord.UserDeviceInfo.DeviceId, {ts, connectionRecord}}); !inserted && iter->second.first < ts) {
                iter->second = {ts, std::move(connectionRecord)};
            }
        }

        TConnectionsStorage::TListConnectionsResult result;
        result.Records.reserve(deviceIdToConnectionRecord.size());
        for (auto& [deviceId, deviceInfo] : deviceIdToConnectionRecord) {
            auto& [ts, connectionRecord] = deviceInfo;
            result.Records.emplace_back(std::move(connectionRecord));
        }

        return result;
    });
}

} // namespace NMatrix::NNotificator
