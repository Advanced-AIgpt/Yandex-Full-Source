#include "push_cards_storage.h"

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>
#include <ydb/public/sdk/cpp/client/ydb_scheme/scheme.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <util/folder/path.h>
#include <util/string/vector.h>
#include <util/system/env.h>

namespace NPersonalCards {

namespace {

// TODO(ndnuriev): remove all this '==' operators after YDBREQUESTS-812

bool operator==(const TVector<NYdb::NTable::TTableColumn>& a, const TVector<NYdb::NTable::TTableColumn>& b) {
    auto convertTableColumns = [](const TVector<NYdb::NTable::TTableColumn>& columns) {
        THashMap<TString, std::pair<TString, TString>> res;
        for (const auto& column : columns) {
            res[column.Name] = {column.Family, column.Type.ToString()};
        }

        return res;
    };

    return convertTableColumns(a) == convertTableColumns(b);
}

bool operator==(const NYdb::NTable::TDateTypeColumnModeSettings& a, const NYdb::NTable::TDateTypeColumnModeSettings& b) {
    return std::tie(a.GetColumnName(), a.GetExpireAfter()) == std::tie(b.GetColumnName(), b.GetExpireAfter());
}

bool operator==(const TMaybe<NYdb::NTable::TTtlSettings>& a, const TMaybe<NYdb::NTable::TTtlSettings>& b) {
    if (!a.Defined() && !b.Defined()) {
        return true;
    }

    return a.Defined() && b.Defined() &&  a->GetMode() == b->GetMode() && a->GetDateTypeColumn() == b->GetDateTypeColumn();
}

NYdb::TDriver CreateDriver(
    const TString& address,
    const TString& dbName,
    const bool preferLocalDc
) {
    auto driverConfig = NYdb::TDriverConfig()
        .SetEndpoint(address)
        .SetDatabase(dbName)
        .SetAuthToken(GetEnv("YDB_TOKEN"))
        .SetBalancingPolicy(preferLocalDc
            ? NYdb::EBalancingPolicy::UsePreferableLocation
            : NYdb::EBalancingPolicy::UseAllNodes
        );

    return NYdb::TDriver(driverConfig);
}

TString StatusToString(const NYdb::TStatus& res) {
    TStringBuilder msg;
    msg << res.GetStatus() << Endl;
    {
        TStringOutput output(msg);
        res.GetIssues().PrintTo(output);
    }

    return msg;
}

class TYdbErrorException : public yexception {
public:
    TYdbErrorException(const NYdb::TStatus& status)
        : Status_(status)
        , Message_(StatusToString(status))
    {}

    const char* what() const noexcept override {
        return Message_.c_str();
    }

    NYdb::TStatus Status_;
    TString Message_;
};

void ThrowOnError(const NYdb::TStatus& res) {
    if (!res.IsSuccess()) {
        throw TYdbErrorException(res);
    }
}

class TPushCardsStorage : public IPushCardsStorage {
public:
    TPushCardsStorage(const TYDBClientConfig& config)
        : TablePrefix_(TString(config.GetDBName()) + TString(config.GetPath()))
        , Driver_(CreateDriver(TString(config.GetAddress()), TString(config.GetDBName()), config.GetPreferLocalDc()))
        , Client_(
            Driver_,
            NYdb::NTable::TClientSettings()
                // https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/public/sdk/cpp/client/ydb_table.h?rev=r7836427#L717
                .UseQueryCache(false)
                .SessionPoolSettings(
                    NYdb::NTable::TSessionPoolSettings()
                        .MaxActiveSessions(config.GetMaxActiveSessions())
                )
        )
        , RetryOperationSettings_(NYdb::NTable::TRetryOperationSettings().MaxRetries(config.GetMaxRetries()))
        , ExecDataQuerySettings_(
            NYdb::NTable::TExecDataQuerySettings()
                .OperationTimeout(FromString<TDuration>(config.GetOperationTimeout()))
                .ClientTimeout(FromString<TDuration>(config.GetClientTimeout()))
                .CancelAfter(FromString<TDuration>(config.GetCancelAfter()))
                .KeepInQueryCache(true)
        )
        , MaxActiveSessionCountInWindow_(0)
    {
        CreatePath(TString(config.GetPath()), TString(config.GetDBName()));
        Y_ENSURE(
            config.GetMinPartitionsCount() <= config.GetMaxPartitionsCount(),
            "Min partitions count greater than max partitions count"
        );
        CreateTable(
            JoinFsPaths(TablePrefix_, TABLE_NAME),
            config.GetPartitioningByLoad(),
            config.GetMinPartitionsCount(),
            config.GetMaxPartitionsCount()
        );
        SetReadReplicasSettings(TString(config.GetReadReplicasSettings()));
    }

    void AddPushCardImpl(
        const TString& uid,
        const TPushCard& pushCard,
        const TInstant sentDateTime,
        const NJson::TJsonValue& data
    ) override {
        ThrowOnError(
            Client_.RetryOperationSync([this, &uid, &pushCard, &sentDateTime, &data](NYdb::NTable::TSession session) {
                UpdateMaxActiveSessionCountInWindow();

                const auto query = Sprintf(R"(
                    --!syntax_v1
                    PRAGMA TablePathPrefix("%s");

                    DECLARE $uid AS String;
                    DECLARE $card_id AS String;
                    DECLARE $tag AS String?;
                    DECLARE $type AS String?;
                    DECLARE $date_from AS Uint32?;
                    DECLARE $date_to AS Uint32;
                    DECLARE $sent_date_time AS Datetime;
                    DECLARE $data AS Json;
                    DECLARE $expire_at AS Timestamp;

                    UPSERT INTO %s (uid, card_id, tag, type, date_from, date_to, sent_date_time, data, expire_at)
                    VALUES ($uid, $card_id, $tag, $type, $date_from, $date_to, $sent_date_time, $data, $expire_at);
                )", TablePrefix_.c_str(), TABLE_NAME.c_str());

                const auto expireAt = TInstant::Seconds(pushCard.date_to()) + PUSH_CARD_TTL;
                const auto dataJson = NJson::WriteJson(
                    data,
                    false, // formatOutput
                    false, // sortkeys
                    false  // validateUtf8
                );
                auto params = session.GetParamsBuilder()
                    .AddParam("$uid")
                        .String(uid)
                        .Build()
                    .AddParam("$card_id")
                        .String(pushCard.card_id())
                        .Build()
                    .AddParam("$tag")
                        .OptionalString(pushCard.has_tag() ? TMaybe<TString>(pushCard.tag()) : Nothing())
                        .Build()
                    .AddParam("$type")
                        .OptionalString(pushCard.has_type() ? TMaybe<TString>(pushCard.type()) : Nothing())
                        .Build()
                    .AddParam("$date_from")
                        .OptionalUint32(pushCard.has_date_from() ? TMaybe<ui32>(pushCard.date_from()) : Nothing())
                        .Build()
                    .AddParam("$date_to")
                        .Uint32(pushCard.date_to())
                        .Build()
                    .AddParam("$sent_date_time")
                        .Datetime(sentDateTime)
                        .Build()
                    .AddParam("$data")
                        .Json(dataJson)
                        .Build()
                    .AddParam("$expire_at")
                        .Timestamp(expireAt)
                        .Build()
                    .Build();

                return session.ExecuteDataQuery(
                    query,
                    NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
                    std::move(params),
                    ExecDataQuerySettings_
                ).GetValueSync();
            }, RetryOperationSettings_)
        );
    }

    void DismissPushCardImpl(const TString& uid) override {
        ThrowOnError(
            Client_.RetryOperationSync([this, &uid](NYdb::NTable::TSession session) {
                UpdateMaxActiveSessionCountInWindow();

                const auto query = Sprintf(R"(
                    --!syntax_v1
                    PRAGMA TablePathPrefix("%s");

                    DECLARE $uid AS String;

                    DELETE FROM %s WHERE uid = $uid;
                )", TablePrefix_.c_str(), TABLE_NAME.c_str());

                auto params = session.GetParamsBuilder()
                    .AddParam("$uid")
                        .String(uid)
                        .Build()
                    .Build();

                return session.ExecuteDataQuery(
                    query,
                    NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
                    std::move(params),
                    ExecDataQuerySettings_
                ).GetValueSync();
            }, RetryOperationSettings_)
        );
    }

    void DismissPushCardImpl(const TString& uid, const TString& cardId) override {
        ThrowOnError(
            Client_.RetryOperationSync([this, &uid, &cardId](NYdb::NTable::TSession session) {
                UpdateMaxActiveSessionCountInWindow();

                const auto query = Sprintf(R"(
                    --!syntax_v1
                    PRAGMA TablePathPrefix("%s");

                    DECLARE $uid AS String;
                    DECLARE $card_id AS String;

                    DELETE FROM %s WHERE uid = $uid AND card_id = $card_id;
                )", TablePrefix_.c_str(), TABLE_NAME.c_str());

                auto params = session.GetParamsBuilder()
                    .AddParam("$uid")
                        .String(uid)
                        .Build()
                    .AddParam("$card_id")
                        .String(cardId)
                        .Build()
                    .Build();

                return session.ExecuteDataQuery(
                    query,
                    NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
                    std::move(params),
                    ExecDataQuerySettings_
                ).GetValueSync();
            }, RetryOperationSettings_)
        );
    }

    NJson::TJsonArray GetPushCardsImpl(const TString& uid, const TInstant now) override {
        TMaybe<NYdb::TResultSet> resultSet;
        ThrowOnError(
            Client_.RetryOperationSync([this, &uid, &resultSet](NYdb::NTable::TSession session) {
                UpdateMaxActiveSessionCountInWindow();

                const auto query = Sprintf(R"(
                    --!syntax_v1
                    PRAGMA TablePathPrefix("%s");

                    DECLARE $uid AS String;

                    SELECT card_id, tag, type, date_from, date_to, data
                    FROM %s
                    WHERE uid = $uid;
                )", TablePrefix_.c_str(), TABLE_NAME.c_str());

                auto params = session.GetParamsBuilder()
                    .AddParam("$uid")
                        .String(uid)
                        .Build()
                    .Build();

                auto res = session.ExecuteDataQuery(
                    query,
                    NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::StaleRO()).CommitTx(),
                    std::move(params),
                    ExecDataQuerySettings_
                ).GetValueSync();

                if (res.IsSuccess()) {
                    resultSet = res.GetResultSet(0);
                }

                return res;
            }, RetryOperationSettings_)
        );

        Y_ENSURE(resultSet, "Result set was not set");

        NYdb::TResultSetParser parser(*resultSet);
        NJson::TJsonArray res;
        while (parser.TryNextRow()) {
            auto optionalToTJsonValue = [](const auto& opt) {
                return opt ? NJson::TJsonValue(*opt) : NJson::TJsonValue(NJson::EJsonValueType::JSON_NULL);
            };
            NJson::TJsonMap cur;
            {
                auto dateFrom = optionalToTJsonValue(parser.ColumnParser("date_from").GetOptionalUint32());
                auto dateTo = optionalToTJsonValue(parser.ColumnParser("date_to").GetOptionalUint32());
                if (dateFrom.IsUInteger() && now < TInstant::Seconds(dateFrom.GetUInteger())) {
                    continue;
                }
                if (dateTo.IsUInteger() && now >= TInstant::Seconds(dateTo.GetUInteger())) {
                    continue;
                }
                cur["date_from"] = std::move(dateFrom);
                cur["date_to"] = std::move(dateTo);
            }

            cur["card_id"] = optionalToTJsonValue(parser.ColumnParser("card_id").GetOptionalString());
            cur["tag"] = optionalToTJsonValue(parser.ColumnParser("tag").GetOptionalString());
            cur["type"] = optionalToTJsonValue(parser.ColumnParser("type").GetOptionalString());
            {
                const auto& data = parser.ColumnParser("data").GetOptionalJson();
                if (data.Defined()) {
                    cur["data"] = NJson::ReadJsonFastTree(*data);
                } else {
                    cur["data"] = NJson::TJsonValue(NJson::EJsonValueType::JSON_NULL);
                }
            }

            res.AppendValue(std::move(cur));
        }
        return res;
    }

    void UpdateSensors() override {
        static NInfra::TIntGaugeSensor gaugeSensor(SENSOR_GROUP, "active_ydb_session_count");
        size_t currentActiveSessionCountInWindow = MaxActiveSessionCountInWindow_.exchange(Client_.GetActiveSessionCount());
        gaugeSensor.Set(currentActiveSessionCountInWindow);
    }

    ~TPushCardsStorage() {
        Driver_.Stop(true /* wait */);
    }

private:
    void CreatePath(const TString& path, const TString& dbName) {
        NYdb::NScheme::TSchemeClient client(Driver_);
        auto parts = SplitString(path, "/");
        auto curPath = TStringBuilder() << dbName;
        for (auto x : parts) {
            curPath << "/" << x;
            ThrowOnError(client.MakeDirectory(curPath).GetValueSync());
        }
    }

    void CreateTable(
        const TString& tablePath,
        const bool partitioningByLoad,
        const ui32 minPartitionsCount,
        const ui32 maxPartitionsCount
    ) {
        ThrowOnError(
            Client_.RetryOperationSync([&](NYdb::NTable::TSession session) -> NYdb::TStatus {
                auto res = session.DescribeTable(tablePath).GetValueSync();
                NYdb::NTable::TPartitioningSettings partitioningSettings;
                auto desc = NYdb::NTable::TTableBuilder()
                    .AddNullableColumn("uid", NYdb::EPrimitiveType::String)
                    .AddNullableColumn("card_id", NYdb::EPrimitiveType::String)
                    .AddNullableColumn("tag", NYdb::EPrimitiveType::String)
                    .AddNullableColumn("type", NYdb::EPrimitiveType::String)
                    .AddNullableColumn("date_from", NYdb::EPrimitiveType::Uint32)
                    .AddNullableColumn("date_to", NYdb::EPrimitiveType::Uint32)
                    .AddNullableColumn("sent_date_time", NYdb::EPrimitiveType::Datetime)
                    .AddNullableColumn("data", NYdb::EPrimitiveType::Json)
                    .AddNullableColumn("expire_at", NYdb::EPrimitiveType::Timestamp)
                    .SetPrimaryKeyColumns({"uid", "card_id"})
                    .SetTtlSettings("expire_at")
                    .SetPartitioningSettings(
                        NYdb::NTable::TPartitioningSettingsBuilder()
                            .SetPartitioningByLoad(partitioningByLoad)
                            .SetMinPartitionsCount(minPartitionsCount)
                            .SetMaxPartitionsCount(maxPartitionsCount)
                            .Build()
                     )
                    .Build();

                if (res.IsSuccess()) {
                    Y_ENSURE(
                        res.GetTableDescription().GetPrimaryKeyColumns() == desc.GetPrimaryKeyColumns(),
                        "Primary key columns of the tables do not match"
                    );
                    Y_ENSURE(
                        res.GetTableDescription().GetTableColumns() == desc.GetTableColumns(),
                        "Columns of the tables do not match"
                    );
                    Y_ENSURE(
                        res.GetTableDescription().GetIndexDescriptions() == desc.GetIndexDescriptions(),
                        "Index descriptions of the tables do not match"
                    );
                    Y_ENSURE(
                        res.GetTableDescription().GetTtlSettings() == desc.GetTtlSettings(),
                        "TTL settings of the tables do not match"
                    );

                    // We don't compare partitioning settings on purpose to be able to change them in RT.

                    return res;
                } else {
                    /*
                     * We can't recognize from the 'DescribeTable' response whether the table exists
                     * so we just try to create the table anyway.
                     */
                    return session.CreateTable(tablePath, std::move(desc)).GetValueSync();
                }
            }, RetryOperationSettings_)
        );
    }

    // Set settings of read-slaves for stale read.
    void SetReadReplicasSettings(const TString settings) {
        ThrowOnError(
            Client_.RetryOperationSync([&](NYdb::NTable::TSession session) -> NYdb::TStatus {
                const auto query = Sprintf(R"(
                    --!syntax_v1
                    PRAGMA TablePathPrefix("%s");

                    ALTER TABLE %s SET READ_REPLICAS_SETTINGS "%s";
                )", TablePrefix_.c_str(), TABLE_NAME.c_str(), settings.c_str());

                return session.ExecuteSchemeQuery(query).GetValueSync();
            }, RetryOperationSettings_)
        );
    }

    void UpdateMaxActiveSessionCountInWindow() {
        ssize_t prevCount = MaxActiveSessionCountInWindow_;
        ssize_t newCount = Client_.GetActiveSessionCount();
        while (prevCount < newCount && MaxActiveSessionCountInWindow_.compare_exchange_weak(prevCount, newCount)) {
        }
    }

private:
    const TString TablePrefix_;
    NYdb::TDriver Driver_;
    NYdb::NTable::TTableClient Client_;
    const NYdb::NTable::TRetryOperationSettings RetryOperationSettings_;
    const NYdb::NTable::TExecDataQuerySettings ExecDataQuerySettings_;
    std::atomic<ssize_t> MaxActiveSessionCountInWindow_;
    inline static const TString TABLE_NAME = "push_cards";
    static constexpr TDuration PUSH_CARD_TTL = TDuration::Days(7);
};

} // namespace

TPushCardsStoragePtr CreateYDBPushCardsStorage(const TYDBClientConfig& ydbClientConfig) {
    return MakeAtomicShared<TPushCardsStorage>(ydbClientConfig);
}

} // namespace NPersonalCards
