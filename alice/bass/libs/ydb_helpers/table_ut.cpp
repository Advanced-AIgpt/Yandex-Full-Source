#include "table.h"
#include "path.h"
#include "queries.h"
#include "testing.h"

#include <alice/bass/libs/ydb_helpers/ut_protos/protos.pb.h>

#include <ydb/public/sdk/cpp/client/ydb_scheme/scheme.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <library/cpp/testing/unittest/registar.h>

#include <google/protobuf/util/message_differencer.h>

#include <util/generic/algorithm.h>
#include <util/generic/maybe.h>
#include <util/string/builder.h>
#include <util/string/printf.h>
#include <util/system/yassert.h>

#include <cstddef>

using namespace NYdbHelpers;

namespace {
template <typename TProto>
TVector<TProto> Deserialize(const NYdb::TResultSet& results) {
    TVector<TProto> rows;
    NYdbHelpers::Deserialize<TProto>(results, [&rows](const TProto& row) { rows.push_back(row); });
    return rows;
}

class TYdbTest : public NUnitTest::TTestBase {
public:
    void SetUp() override {
        Db.Init();

        Y_ASSERT(Db.Driver);
        TableClient = MakeHolder<NYdb::NTable::TTableClient>(*Db.Driver);
        SchemeClient = MakeHolder<NYdb::NScheme::TSchemeClient>(*Db.Driver);
    }

    void TearDown() override {
        TableClient.Reset();
        SchemeClient.Reset();
    }

    UNIT_TEST_SUITE(TYdbTest);
    UNIT_TEST(Smoke);
    UNIT_TEST(Upsert);
    UNIT_TEST(List);
    UNIT_TEST(ProtoMismatch);
    UNIT_TEST_SUITE_END();

    void Smoke();
    void Upsert();
    void List();
    void ProtoMismatch();

private:
    TLocalDatabase Db;
    THolder<NYdb::NTable::TTableClient> TableClient;
    THolder<NYdb::NScheme::TSchemeClient> SchemeClient;
};
UNIT_TEST_SUITE_REGISTRATION(TYdbTest);

void TYdbTest::Smoke() {
    UNIT_ASSERT(TableClient);
    auto& client = *TableClient;

    const TTablePath path{Db.Database, "video_items"};

    UNIT_ASSERT(!CheckTableExists(client, path).IsSuccess());

    auto table = TScopedTable::Make<TVideoItem>(client, path, {"Id"} /* primaryKeys */);
    CheckTableExistsOrFail(client, path);

    DropTableOrFail(client, path);
    UNIT_ASSERT(!CheckTableExists(client, path).IsSuccess());

    table.Release();
}

void TYdbTest::Upsert() {
    UNIT_ASSERT(TableClient);
    auto& client = *TableClient;

    const TTablePath path{Db.Database, "video_items"};

    UNIT_ASSERT(!CheckTableExists(client, path).IsSuccess());

    auto table = TScopedTable::Make<TVideoItem>(client, path, {"Id"});
    CheckTableExistsOrFail(client, path);

    TUpsertQuery<TVideoItem> query(path);

    TVideoItem item0;
    item0.SetId("masha-and-the-bear");
    item0.SetName("Маша и медведь");
    item0.SetRating(10);
    item0.SetMinAge(0);
    query.Add(item0);

    TVideoItem item1;
    item1.SetId("ubrat-periskop");
    item1.SetName("Убрать перископ");
    item1.SetRating(10);
    item1.SetMinAge(16);
    query.Add(item1);

    TVideoItem item2;
    item2.SetId("killer-tomatoes");
    item2.SetName("Нападение помидоров убийц");
    item2.SetRating(2.0);
    item2.SetMinAge(16);
    query.Add(item2);

    ExecuteDataQueryOrFail(client, query, SerializableRW());

    {
        const auto query = Sprintf(R"(
                                   PRAGMA TablePathPrefix("%s");
                                   SELECT * FROM [%s] WHERE MinAge >= 16;
                                   )",
                                   path.Database.c_str(), path.Name.c_str());

        TMaybe<NYdb::NTable::TDataQueryResult> result;
        const auto status = client.RetryOperationSync([&](NYdb::NTable::TSession session) {
            result = session.ExecuteDataQuery(query, SerializableRW()).GetValueSync();
            return *result;
        });
        UNIT_ASSERT(status.IsSuccess());
        UNIT_ASSERT(result.Defined());
        UNIT_ASSERT_VALUES_EQUAL(result->GetResultSets().size(), 1);

        auto items = Deserialize<TVideoItem>(result->GetResultSet(0));
        UNIT_ASSERT_VALUES_EQUAL(items.size(), 2);

        Sort(items, [](const TVideoItem& lhs, const TVideoItem& rhs) { return lhs.GetId() < rhs.GetId(); });

        UNIT_ASSERT(google::protobuf::util::MessageDifferencer::Equals(items[0], item2));
        UNIT_ASSERT(google::protobuf::util::MessageDifferencer::Equals(items[1], item1));
    }
}

void TYdbTest::List() {
    UNIT_ASSERT(TableClient);
    auto& tableClient = *TableClient;

    UNIT_ASSERT(SchemeClient);
    auto& schemeClient = *SchemeClient;

    {
        TVector<TString> tables;
        const auto status =
            ListTables(schemeClient, Db.Database, [&](const TString& path) { tables.push_back(path); });
        UNIT_ASSERT(status.IsSuccess());
        UNIT_ASSERT(tables.empty());
    }

    {
        TVector<TString> tables;
        const auto status = ListTables(schemeClient, "/this_path_does_not_exist",
                                       [&](const TString& path) { tables.push_back(path); });
        UNIT_ASSERT(!status.IsSuccess());
        UNIT_ASSERT(tables.empty());
    }

    const auto tableA = TScopedTable::Make<TVideoItem>(tableClient, TTablePath{Db.Database, "table_a"}, {"Id"});
    const auto tableB = TScopedTable::Make<TVideoItem>(tableClient, TTablePath{Db.Database, "table_b"}, {"Id"});

    {
        TVector<TString> tables;
        const auto status =
            ListTables(schemeClient, Db.Database, [&](const TString& path) { tables.push_back(path); });
        UNIT_ASSERT(status.IsSuccess());

        Sort(tables);
        UNIT_ASSERT_VALUES_EQUAL(tables, TVector<TString>({"table_a", "table_b"}));
    }
}

void TYdbTest::ProtoMismatch() {
    const size_t kNumRows = 10;

    UNIT_ASSERT(TableClient);
    auto& client = *TableClient;

    const TTablePath path{Db.Database, "table"};

    UNIT_ASSERT(!CheckTableExists(client, path).IsSuccess());

    const auto table = TScopedTable::Make<TOldRow>(client, path, {"Key"});

    {
        TTableWriter<TOldRow> writer{client, path};

        for (size_t i = 0; i < kNumRows; ++i) {
            TOldRow row;
            row.SetKey(TStringBuilder() << "Key" << i);
            row.SetValue(TStringBuilder() << "Value" << i);
            writer.AddRow(row);
        }
    }

    const auto query = Sprintf(R"(
                               PRAGMA TablePathPrefix("%s");
                               SELECT * FROM [%s];
                               )",
                               path.Database.c_str(), path.Name.c_str());
    TMaybe<NYdb::NTable::TDataQueryResult> result;
    const auto status = client.RetryOperationSync([&](NYdb::NTable::TSession session) {
        result = session.ExecuteDataQuery(query, SerializableRW()).GetValueSync();
        return *result;
    });

    UNIT_ASSERT(status.IsSuccess());
    UNIT_ASSERT(result.Defined());
    UNIT_ASSERT_VALUES_EQUAL(result->GetResultSets().size(), 1);

    UNIT_ASSERT_EXCEPTION(Deserialize<TNewRowStrict>(result->GetResultSet(0)), TBadArgumentException);

    {
        const auto rows = Deserialize<TNewRow>(result->GetResultSet(0));
        UNIT_ASSERT_VALUES_EQUAL(rows.size(), kNumRows);
    }

    {
        const auto rows = Deserialize<TNewRowSmall>(result->GetResultSet(0));
        UNIT_ASSERT_VALUES_EQUAL(rows.size(), kNumRows);
    }
}
} // namespace
