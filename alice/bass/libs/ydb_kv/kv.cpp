#include "kv.h"

#include <alice/bass/libs/ydb_kv/protos/kv.pb.h>

#include <alice/bass/libs/ydb_helpers/table.h>

#include <util/system/yassert.h>

#include <cstddef>

using namespace NYdbHelpers;

namespace NYdbKV {
namespace {
size_t ParseKVs(NYdb::TResultSetParser parser, TVector<TKeyValue>& kvs) {
    size_t numRows = 0;

    while (parser.TryNextRow()) {
        const auto key = parser.ColumnParser(0).GetOptionalString();
        const auto value = parser.ColumnParser(1).GetOptionalString();

        if (!key)
            continue;

        TKeyValue kv;

        kv.SetKey(*key);
        if (value)
            kv.SetValue(*value);

        kvs.push_back(kv);
        ++numRows;
    }

    return numRows;
}

template <typename TParamsBuilder>
NYdb::TStatus ExecuteDataQuery(NYdb::NTable::TTableClient& client, const TString& query, TParamsBuilder&& paramsBuilder) {
    return RetrySyncWithDefaultSettings(client, [&](NYdb::NTable::TSession session) -> NYdb::TStatus {
        auto prepareResult = session.PrepareDataQuery(query).GetValueSync();
        if (!prepareResult.IsSuccess())
            return prepareResult;

        auto preparedQuery = prepareResult.GetQuery();
        auto params = paramsBuilder(session);
        return preparedQuery.Execute(SerializableRW(), std::move(params)).GetValueSync();
    });
}
} // namespace

// -----------------------------------------------------------------------------
TKeyValue MakeKeyValue(const TString& key, const TMaybe<TString>& value) {
    TKeyValue kv;
    kv.SetKey(key);
    if (value)
        kv.SetValue(*value);
    return kv;
}

// TKV -------------------------------------------------------------------------
TKV::TKV(NYdb::NTable::TTableClient& client, const TTablePath& path,
         const NYdb::NTable::TRetryOperationSettings& settings)
    : Client(client)
    , Path(path)
    , Settings(settings) {
    const auto type = NYdbHelpers::ProtoToStructType<TKeyValue>();

    GetQuery = Sprintf(R"(
                       PRAGMA TablePathPrefix("%s");
                       DECLARE $key AS String;
                       SELECT Value FROM [%s] WHERE Key == $key;
                       )",
                       path.Database.c_str(), path.Name.c_str());
    GetAllQuery = Sprintf(R"(
                          PRAGMA TablePathPrefix("%s");
                          SELECT Key, Value FROM [%s] ORDER BY Key;
                          )",
                          path.Database.c_str(), path.Name.c_str());
    GetAllContQuery = Sprintf(R"(
                              PRAGMA TablePathPrefix("%s");
                              DECLARE $lastKey AS String;
                              SELECT Key, Value FROM [%s] WHERE Key > $lastKey ORDER BY Key;
                              )",
                              path.Database.c_str(), path.Name.c_str());
    SetQuery = Sprintf(R"(
                       PRAGMA TablePathPrefix("%s");
                       DECLARE $key AS String;
                       DECLARE $value AS String;
                       UPSERT INTO [%s] (Key, Value) VALUES ($key, $value);
                       )",
                       path.Database.c_str(), path.Name.c_str());
    SetManyQuery = Sprintf(R"(
                           PRAGMA TablePathPrefix("%s");
                           DECLARE $kvs AS "List<%s>";
                           UPSERT INTO [%s] SELECT * FROM AS_TABLE($kvs);
                           )",
                           path.Database.c_str(), NYdb::FormatType(type).c_str(), path.Name.c_str());
    DeleteQuery = Sprintf(R"(
                          PRAGMA TablePathPrefix("%s");
                          DECLARE $key AS String;
                          DELETE FROM [%s] WHERE Key == $key;
                          )",
                       path.Database.c_str(), path.Name.c_str());
}

NYdb::TStatus TKV::Create() {
    return CreateTable<TKeyValue>(Client, Path, {"Key"} /* primaryKeys */, Settings);
}

NYdb::TStatus TKV::Drop() {
    return DropTable(Client, Path, Settings);
}

NYdb::TStatus TKV::Exists() {
    return CheckTableExists(Client, Path, Settings);
}

TValueStatus TKV::Get(TStringBuf key) {
    TMaybe<TString> value;

    const auto status = Client.RetryOperationSync(
        [this, &key, &value](NYdb::NTable::TSession session) -> NYdb::TStatus {
            auto prepareResult = session.PrepareDataQuery(GetQuery).GetValueSync();
            if (!prepareResult.IsSuccess()) {
                return prepareResult;
            }

            auto preparedQuery = prepareResult.GetQuery();
            auto params = session.GetParamsBuilder().AddParam("$key").String(TString{key}).Build().Build();

            const auto result = preparedQuery.Execute(SerializableRW(), std::move(params)).GetValueSync();
            if (!result.IsSuccess())
                return result;

            Y_ASSERT(result.GetResultSets().size() == 1);
            auto parser = result.GetResultSetParser(0);

            if (!parser.TryNextRow())
                return result;

            Y_ASSERT(parser.ColumnsCount() == 1);
            value = parser.ColumnParser(0).GetOptionalString();
            return result;
        },
        Settings);

    return TValueStatus{std::move(status), value};
}

TKeyValuesStatus TKV::GetAll() {
    TVector<TKeyValue> keyValues;

    auto status = Client.RetryOperationSync(
        [&](NYdb::NTable::TSession session) -> NYdb::TStatus {
            auto prepareResult = session.PrepareDataQuery(GetAllQuery).GetValueSync();
            if (!prepareResult.IsSuccess())
                return prepareResult;

            auto preparedQuery = prepareResult.GetQuery();
            auto result = preparedQuery.Execute(SerializableRW()).GetValueSync();
            if (!result.IsSuccess())
                return result;

            Y_ASSERT(result.GetResultSets().size() == 1);

            size_t readRows = ParseKVs(result.GetResultSetParser(0), keyValues);

            if (keyValues.empty()) {
                Y_ASSERT(readRows == 0);
                return result;
            }

            prepareResult = session.PrepareDataQuery(GetAllContQuery).GetValueSync();
            if (!prepareResult.IsSuccess())
                return prepareResult;

            while (readRows != 0) {
                auto preparedQuery = prepareResult.GetQuery();
                const TString lastKey = keyValues.back().GetKey();
                auto params = session.GetParamsBuilder().AddParam("$lastKey").String(lastKey).Build().Build();
                result = preparedQuery.Execute(SerializableRW(), std::move(params)).GetValueSync();
                if (!result.IsSuccess())
                    return result;

                Y_ASSERT(result.GetResultSets().size() == 1);
                readRows = ParseKVs(result.GetResultSetParser(0), keyValues);
            }

            return result;
        },
        Settings);

    return TKeyValuesStatus{std::move(status), std::move(keyValues)};
}

NYdb::TStatus TKV::Set(TStringBuf key, TStringBuf value) {
    return ExecuteDataQuery(Client, SetQuery, [&](NYdb::NTable::TSession session)
    {
        return session.GetParamsBuilder()
            .AddParam("$key")
                .String(TString{key})
                .Build()
            .AddParam("$value")
                .String(TString{value})
                .Build()
            .Build();
    });
}

NYdb::TStatus TKV::SetMany(const TVector<TKeyValue>& keyValues) {
    return ExecuteDataQuery(Client, SetManyQuery, [&](NYdb::NTable::TSession session)
    {
        const auto kvs = ProtosToList(keyValues);
        return session.GetParamsBuilder().AddParam("$kvs", kvs).Build();
    });
}

NYdb::TStatus TKV::Delete(TStringBuf key) {
    return ExecuteDataQuery(Client, DeleteQuery, [&](NYdb::NTable::TSession session)
    {
        return session.GetParamsBuilder().AddParam("$key").String(TString{key}).Build().Build();
    });
}
} // namespace NYdbKV
