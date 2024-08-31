#include "storage.h"

#include <util/digest/city.h>
#include <util/string/printf.h>

namespace NMatrix::NNotificator {

TDirectivesStorage::TDirectivesStorage(
    const NYdb::TDriver& driver,
    const TYDBClientSettings& config
)
    : IYDBStorage(driver, config, NAME)
{}

NThreading::TFuture<TExpected<TVector<TDirectivesStorage::TDirective>, TString>> TDirectivesStorage::GetDirectives(
    const TUserDevice& userDevice,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "get_directives";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, userDevice, resultSet](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;
            DECLARE $device_id AS String;
            DECLARE $expired_at AS Datetime;
            DECLARE $status AS Uint32;

            SELECT push_id, directive, created_at
                FROM directives
            WHERE shard_key = $shard_key
                AND yandexid = $yandexid
                AND device_id = $device_id
                AND status = $status
                AND expired_at >= $expired_at
            ORDER BY created_at;
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(CityHash64(userDevice.Puid))
                .Build()
            .AddParam("$yandexid")
                .String(userDevice.Puid)
                .Build()
            .AddParam("$device_id")
                .String(userDevice.DeviceId)
                .Build()
            .AddParam("$expired_at")
                .Datetime(GetDirectiveExpiredAt(TInstant::Now()))
                .Build()
            .AddParam("$status")
                .Uint32(NAlice::NNotificator::EDirectiveStatus::ED_NEW)
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
    }, RetryOperationSettings_).Apply([resultSet, operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<TVector<TDirective>, TString> {
        const auto& status = fut.GetValueSync();
        if (const auto opRes = operationContext.ReportResult(status); !opRes) {
            return opRes.Error();
        }

        NYdb::TResultSetParser parser(resultSet->GetRef());
        TVector<TDirective> result;
        result.reserve(parser.RowsCount());
        while (parser.TryNextRow()) {
            TDirective directive;

            directive.PushId = *parser.ColumnParser("push_id").GetOptionalString();

            auto protoParseRes = operationContext.ParseProtoFromStringWithErrorReport(directive.SpeechKitDirective, *parser.ColumnParser("directive").GetOptionalString());
            if (protoParseRes.IsError()) {
                return protoParseRes.Error();
            }

            result.emplace_back(directive);
        }

        return result;
    });
}


NThreading::TFuture<TExpected<TDirectivesStorage::TGetDirectivesMultiUserDevicesResult, TString>> TDirectivesStorage::GetDirectivesMultiUserDevices(
    const TVector<TDirectivesStorage::TUserDevice>& userDevices,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "get_directives_multi_user_devices";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, userDevices, resultSet](NYdb::NTable::TSession session) {
        static const auto query = Sprintf(R"(
            --!syntax_v1

            PRAGMA AnsiInForEmptyOrNullableItemsCollections;

            DECLARE $expired_at AS Datetime;
            DECLARE $status AS Uint32;

            DECLARE $user_devices AS List<
                Tuple<
                    Uint64,
                    String,
                    String,
                >
            >;

            SELECT yandexid, device_id, push_id, directive, created_at FROM directives
            WHERE
                (shard_key, yandexid, device_id) IN COMPACT $user_devices
                AND status = $status
                AND expired_at >= $expired_at
            ORDER BY created_at
            LIMIT %u;
        )", MAX_DIRECTIVES_IN_GET_DIRECTIVES_MULTI_USER_DEVICES);

        auto paramsBuilder = session.GetParamsBuilder();
        paramsBuilder
            .AddParam("$expired_at")
                .Datetime(GetDirectiveExpiredAt(TInstant::Now()))
                .Build()
            .AddParam("$status")
                .Uint32(NAlice::NNotificator::EDirectiveStatus::ED_NEW)
                .Build();

        {
            auto& param = paramsBuilder.AddParam("$user_devices");

            if (userDevices.empty()) {
                param.EmptyList(
                    NYdb::TTypeBuilder()
                        .BeginTuple()
                            .AddElement().Primitive(NYdb::EPrimitiveType::Uint64)
                            .AddElement().Primitive(NYdb::EPrimitiveType::String)
                            .AddElement().Primitive(NYdb::EPrimitiveType::String)
                        .EndTuple()
                    .Build()
                );
            } else {
                param.BeginList();
                for (const auto& userDevice : userDevices) {
                    param.AddListItem()
                        .BeginTuple()
                            .AddElement().Uint64(CityHash64(userDevice.Puid))
                            .AddElement().String(userDevice.Puid)
                            .AddElement().String(userDevice.DeviceId)
                        .EndTuple();
                }
                param.EndList();
            }

            param.Build();
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
    }, RetryOperationSettings_).Apply([resultSet, operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<TGetDirectivesMultiUserDevicesResult, TString> {
        const auto& status = fut.GetValueSync();
        if (const auto opRes = operationContext.ReportResult(status); !opRes) {
            return opRes.Error();
        }

        TGetDirectivesMultiUserDevicesResult result;

        NYdb::TResultSetParser parser(resultSet->GetRef());
        result.UserDirectives.reserve(parser.RowsCount());
        while (parser.TryNextRow()) {
            TUserDirective userDirective;

            // Parse directive first for fast error report
            auto protoParseRes = operationContext.ParseProtoFromStringWithErrorReport(
                userDirective.Directive.SpeechKitDirective,
                *parser.ColumnParser("directive").GetOptionalString()
            );
            if (protoParseRes.IsError()) {
                return protoParseRes.Error();
            }

            userDirective.UserDevice.Puid = *parser.ColumnParser("yandexid").GetOptionalString();
            userDirective.UserDevice.DeviceId = *parser.ColumnParser("device_id").GetOptionalString();
            userDirective.Directive.PushId = *parser.ColumnParser("push_id").GetOptionalString();

            result.UserDirectives.emplace_back(std::move(userDirective));
        }

        // Assume that exacly MAX_DIRECTIVES_IN_GET_DIRECTIVES_MULTI_USER_DEVICES directives is truncated result
        // It's too expensive to get real number of directives in full result
        result.IsTruncated = (result.UserDirectives.size() == MAX_DIRECTIVES_IN_GET_DIRECTIVES_MULTI_USER_DEVICES);
        return result;
    });
}

NThreading::TFuture<TExpected<NAlice::NNotificator::EDirectiveStatus, TString>> TDirectivesStorage::GetDirectiveStatus(
    const NAlice::NNotificator::TDirectiveStatus& msg,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "get_directive_status";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    auto resultSet = MakeAtomicShared<TMaybe<NYdb::TResultSet>>();
    return Client_.RetryOperation([this, resultSet, msg](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;
            DECLARE $device_id AS String;
            DECLARE $id AS String;

            SELECT status, expired_at
                FROM directives
            WHERE shard_key = $shard_key
                AND yandexid  = $yandexid
                AND device_id = $device_id
                AND push_id = $id;
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(CityHash64(msg.GetPuid()))
                .Build()
            .AddParam("$yandexid")
                .String(msg.GetPuid())
                .Build()
            .AddParam("$device_id")
                .String(msg.GetDeviceId())
                .Build()
            .AddParam("$id")
                .String(msg.GetId())
                .Build()
            .Build();

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::OnlineRO()).CommitTx(),
            std::move(params),
            ExecDataQuerySettings_
        ).Apply([resultSet](const NYdb::NTable::TAsyncDataQueryResult& fut) -> NYdb::TStatus {
            const auto& res = fut.GetValueSync();
            if (res.IsSuccess()) {
                *resultSet = res.GetResultSet(0);
            }

            return res;
        });
    }, RetryOperationSettings_).Apply([resultSet, operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable -> TExpected<NAlice::NNotificator::EDirectiveStatus, TString> {
        const auto& status = fut.GetValueSync();
        if (const auto opRes = operationContext.ReportResult(status); !opRes) {
            return opRes.Error();
        }

        NYdb::TResultSetParser parser(resultSet->GetRef());
        if (parser.TryNextRow()) {
            ui32 ui32Status = *parser.ColumnParser("status").GetOptionalUint32();
            TInstant expiredAt = *parser.ColumnParser("expired_at").GetOptionalTimestamp();

            if (!NAlice::NNotificator::EDirectiveStatus_IsValid(ui32Status)) {
                return TString::Join("Status '", ToString(ui32Status), "' is invalid");
            }
            NAlice::NNotificator::EDirectiveStatus status = static_cast<NAlice::NNotificator::EDirectiveStatus>(ui32Status);

            if (status == NAlice::NNotificator::EDirectiveStatus::ED_NEW && expiredAt < GetDirectiveExpiredAt(TInstant::Now())) {
                return NAlice::NNotificator::EDirectiveStatus::ED_EXPIRED;
            }

            return status;
        } else {
            return NAlice::NNotificator::EDirectiveStatus::ED_NOT_FOUND;
        }
    });
}

NThreading::TFuture<TExpected<void, TString>> TDirectivesStorage::AddDirective(
    const TUserDevice& userDevice,
    const TDirective& directive,
    const TDuration ttl,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "add_directive";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, userDevice, directive, ttl](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;
            DECLARE $device_id AS String;
            DECLARE $push_id AS String;
            DECLARE $directive AS String;
            DECLARE $status AS Uint32;
            DECLARE $created_at AS Datetime;
            DECLARE $expired_at AS Datetime;

            UPSERT INTO directives
                (shard_key, yandexid, device_id, push_id, directive, status, expired_at, created_at)
            VALUES
                ($shard_key, $yandexid, $device_id, $push_id, $directive, $status, $expired_at, $created_at);
        )";

        const auto now = TInstant::Now();
        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(CityHash64(userDevice.Puid))
                .Build()
            .AddParam("$yandexid")
                .String(userDevice.Puid)
                .Build()
            .AddParam("$device_id")
                .String(userDevice.DeviceId)
                .Build()
            .AddParam("$push_id")
                .String(directive.PushId)
                .Build()
            .AddParam("$directive")
                .String(directive.SpeechKitDirective.SerializeAsString())
                .Build()
            .AddParam("$status")
                .Uint32(NAlice::NNotificator::EDirectiveStatus::ED_NEW)
                .Build()
            .AddParam("$created_at")
                .Datetime(now)
                .Build()
            .AddParam("$expired_at")
                .Datetime(GetDirectiveExpiredAt(now + Min(ttl, MAX_DIRECTIVE_TTL)))
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

NThreading::TFuture<TExpected<void, TString>> TDirectivesStorage::ChangeDirectivesStatus(
    const NAlice::NNotificator::TChangeStatus& msg,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    static const TString operationName = "change_directives_status";
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

            DECLARE $shard_key AS Uint64;
            DECLARE $yandexid AS String;
            DECLARE $device_id AS String;
            DECLARE $status AS Uint32;
            DECLARE $ids AS List<String>;

            UPDATE directives
                SET status = $status
            WHERE shard_key = $shard_key
                AND yandexid = $yandexid
                AND device_id = $device_id
                AND push_id in $ids;
        )";

        auto paramsBuilder = session.GetParamsBuilder();
        paramsBuilder
            .AddParam("$shard_key")
                .Uint64(CityHash64(msg.GetPuid()))
                .Build()
            .AddParam("$yandexid")
                .String(msg.GetPuid())
                .Build()
            .AddParam("$device_id")
                .String(msg.GetDeviceId())
                .Build()
            .AddParam("$status")
                .Uint32(static_cast<ui32>(msg.GetStatus()))
                .Build();

        auto& param = paramsBuilder.AddParam("$ids");
        if (msg.GetIds().empty()) {
            param.EmptyList(NYdb::TTypeBuilder().Primitive(NYdb::EPrimitiveType::String).Build()).Build();
        } else {
            param.BeginList();
            for (const auto& id : msg.GetIds()) {
                param.AddListItem(NYdb::TValueBuilder().String(id).Build());
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

NThreading::TFuture<TExpected<void, TString>> TDirectivesStorage::RemoveAllUserData(
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

            DELETE FROM directives
            WHERE
                shard_key = $shard_key AND
                yandexid = $yandexid
            ;
        )";

        auto params = session.GetParamsBuilder()
            .AddParam("$shard_key")
                .Uint64(CityHash64(puid))
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

TInstant TDirectivesStorage::GetDirectiveExpiredAt(TInstant realExpiredAt) {
    return realExpiredAt + DIRECTIVE_TTL_AFTER_EXPIRATION;
}

} // namespace NMatrix::NNotificator
