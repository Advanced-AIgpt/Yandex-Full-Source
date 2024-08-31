#include "kinopoisk_utils.h"


#include <alice/bass/libs/video_content/common.h>
#include <alice/bass/libs/video_content/protos/rows.pb.h>
#include <alice/bass/libs/ydb_helpers/table.h>

#include <alice/bass/libs/ut_helpers/test_with_ydb.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>
#include <util/string/builder.h>

using namespace NVideoCommon::NTesting;
using namespace NVideoCommon;
using namespace NVideoContent::NProtos;
using namespace NVideoContent;
using namespace NYdbHelpers;

namespace {
class TKinopoiskUtilsTest : public TestWithYDB {
public:
    TStringBuf GetTablesPrefix() const {
        return KINOPOISK_SVOD_V2_TABLE_PREFIX;
    }

    UNIT_TEST_SUITE(TKinopoiskUtilsTest);
    UNIT_TEST(Smoke);
    UNIT_TEST(DropOldTables);
    UNIT_TEST_SUITE_END();

    void Smoke();
    void DropOldTables();
};
UNIT_TEST_SUITE_REGISTRATION(TKinopoiskUtilsTest);

void TKinopoiskUtilsTest::Smoke() {
    UNIT_ASSERT(TableClient);
    auto& client = *TableClient;

    const TTablePath path{Db.Database, NVideoCommon::KINOPOISK_DEFAULT_SVOD_TABLE};
    UNIT_ASSERT(!CheckTableExists(client, path).IsSuccess());

    const auto table = TScopedTable::Make<TYDBKinopoiskSVODRowV2>(client, path, VIDEO_TABLE_KINOPOISK_PRIMARY_KEYS);

    {
        TTableWriter<TYDBKinopoiskSVODRowV2> writer{client, path};
        for (const auto& key : {"0001", "0002", "0004", "0042"}) {
            TYDBKinopoiskSVODRowV2 row;
            row.SetKinopoiskId(key);
            writer.AddRow(row);
        }
    }

    {
        TVector<TString> out;
        const auto error = RemoveIfNotKinopoiskSVOD(client, path, {} /* in */, out);
        UNIT_ASSERT_C(!error.Defined(), *error);
        UNIT_ASSERT(out.empty());
    }

    {
        const TVector<TString> in{"0001"};
        TVector<TString> out;
        const auto error = RemoveIfNotKinopoiskSVOD(client, path, in, out);
        UNIT_ASSERT_C(!error.Defined(), *error);
        UNIT_ASSERT_VALUES_EQUAL(out, in);
    }

    {
        const TVector<TString> in{"0003"};
        TVector<TString> out;
        const auto error = RemoveIfNotKinopoiskSVOD(client, path, in, out);
        UNIT_ASSERT_C(!error.Defined(), *error);
        UNIT_ASSERT(out.empty());
    }

    const TVector<TString> ids = {{"0001", "0002", "0003", "0004", "0005", "0006"}};
    const TVector<TString> expected = {{"0001", "0002", "0004"}};

    TVector<TString> actual;

    const auto error = RemoveIfNotKinopoiskSVOD(client, path, ids, actual);
    UNIT_ASSERT_C(!error.Defined(), *error);
    UNIT_ASSERT_VALUES_EQUAL(actual, expected);

    {
        const TIsSVODStatus status = IsKinopoiskSVOD(client, path, "0001");
        UNIT_ASSERT(status.IsSuccess());
        UNIT_ASSERT(status.IsSVOD());
    }
    {
        const TIsSVODStatus status = IsKinopoiskSVOD(client, path, "0005");
        UNIT_ASSERT(status.IsSuccess());
        UNIT_ASSERT(!status.IsSVOD());
    }
    {
        const TIsSVODStatus status = IsKinopoiskSVOD(client, path, "0042");
        UNIT_ASSERT(status.IsSuccess());
        UNIT_ASSERT(status.IsSVOD());
    }
}

void TKinopoiskUtilsTest::DropOldTables() {
    UNIT_ASSERT(TableClient);
    auto& tableClient = *TableClient;

    UNIT_ASSERT(SchemeClient);
    auto& schemeClient = *SchemeClient;

    TVector<TScopedTable> tables;
    for (size_t i = 0; i < 5; ++i) {
        const NYdbHelpers::TTablePath path{Db.Database, TStringBuilder() << GetTablesPrefix() << i};
        tables.emplace_back(
            TScopedTable::Make<TYDBKinopoiskSVODRowV2>(tableClient, path, VIDEO_TABLE_KINOPOISK_PRIMARY_KEYS));
    }

    for (const auto& table : tables)
        UNIT_ASSERT(CheckTableExists(tableClient, table.Path()).IsSuccess());

    UNIT_ASSERT(DropOldKinopoiskSVODTables(schemeClient, tableClient, Db.Database, GetTablesPrefix(),
                                           "kinopoisk_svod_latest"));

    UNIT_ASSERT_VALUES_EQUAL(tables.size(), 5);
    UNIT_ASSERT(!CheckTableExists(tableClient, tables[0].Path()).IsSuccess());
    UNIT_ASSERT(!CheckTableExists(tableClient, tables[1].Path()).IsSuccess());
    UNIT_ASSERT(!CheckTableExists(tableClient, tables[2].Path()).IsSuccess());
    UNIT_ASSERT(CheckTableExists(tableClient, tables[3].Path()).IsSuccess());
    UNIT_ASSERT(CheckTableExists(tableClient, tables[4].Path()).IsSuccess());
}
} // namespace
