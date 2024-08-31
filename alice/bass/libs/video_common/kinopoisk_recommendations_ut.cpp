#include "kinopoisk_recommendations.h"

#include <library/cpp/testing/unittest/registar.h>

#include <alice/bass/libs/video_common/ut/protos/rows.pb.h>
#include <alice/bass/libs/video_content/common.h>
#include <alice/bass/libs/video_content/protos/rows.pb.h>

#include <alice/bass/libs/ut_helpers/test_with_ydb.h>
#include <alice/bass/libs/ydb_helpers/exception.h>

#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/ymath.h>
#include <util/string/builder.h>

#include <cstddef>
#include <utility>

using namespace NVideoCommon::NTesting::NProtos;
using namespace NVideoCommon::NTesting;
using namespace NVideoCommon;
using namespace NVideoContent::NProtos;
using namespace NVideoContent;
using namespace NYdbHelpers;

namespace {
using TYDBKinopoiskSVODRow = TYDBKinopoiskSVODRowV2;

class TKinopoiskRecommendationsTest : public TestWithYDB {
public:
    UNIT_TEST_SUITE(TKinopoiskRecommendationsTest);
    UNIT_TEST(Smoke);
    UNIT_TEST(SerDes);
    UNIT_TEST(Many);
    UNIT_TEST(BadData);
    UNIT_TEST_SUITE_END();

    void Smoke();
    void SerDes();
    void Many();
    void BadData();
};
UNIT_TEST_SUITE_REGISTRATION(TKinopoiskRecommendationsTest);

void TKinopoiskRecommendationsTest::Smoke() {
    UNIT_ASSERT(TableClient);

    const auto table = TScopedTable::Make<TYDBKinopoiskSVODRow>(
        *TableClient, NYdbHelpers::TTablePath{Db.Database, "kinopoisk"}, VIDEO_TABLE_KINOPOISK_PRIMARY_KEYS);

    const auto status = GetFilmsInfos(*TableClient, table.Path());
    ThrowOnError(status);

    UNIT_ASSERT(status.GetInfos().empty());
}

void TKinopoiskRecommendationsTest::SerDes() {
    UNIT_ASSERT(TableClient);

    const auto table = TScopedTable::Make<TYDBKinopoiskSVODRow>(
        *TableClient, NYdbHelpers::TTablePath{Db.Database, "kinopoisk"}, VIDEO_TABLE_KINOPOISK_PRIMARY_KEYS);

    TVector<TKinopoiskFilmInfo> expectedInfos;

    {
        TKinopoiskFilmInfo info;
        info.Id = "12345";
        info.Rating = 9.9;
        info.Genres = TVector<EVideoGenre>{EVideoGenre::Action, EVideoGenre::Adventure};
        info.Countries.push_back("Россия");
        info.ContentType = EContentType::Movie;
        expectedInfos.push_back(std::move(info));
    }

    {
        TKinopoiskFilmInfo info;
        info.Id = "67890";
        info.Rating = 7.0;
        info.Genres = TVector<EVideoGenre>{EVideoGenre::Arthouse, EVideoGenre::Noir, EVideoGenre::Thriller};
        info.Countries.push_back("Франция");
        info.ContentType = EContentType::Cartoon;
        expectedInfos.push_back(std::move(info));
    }

    {
        TKinopoiskFilmInfo info;
        info.Id = "02468";
        info.ContentType = EContentType::TvShow;
        expectedInfos.push_back(std::move(info));
    }

    {
        TTableWriter<TYDBKinopoiskSVODRow> writer{*TableClient, table.Path()};
        for (const auto& info : expectedInfos) {
            TYDBKinopoiskSVODRow row;
            info.Ser(row);
            writer.AddRow(row);
        }
    }

    Sort(expectedInfos, [&](const TKinopoiskFilmInfo& lhs, const TKinopoiskFilmInfo& rhs) { return lhs.Id < rhs.Id; });

    const auto status = GetFilmsInfos(*TableClient, table.Path());
    ThrowOnError(status);
    const auto& actualInfos = status.GetInfos();
    UNIT_ASSERT_VALUES_EQUAL(actualInfos.size(), expectedInfos.size());

    for (size_t i = 0; i < expectedInfos.size(); ++i) {
        const auto& actual = actualInfos[i];
        const auto& expected = expectedInfos[i];
        UNIT_ASSERT_C(actual.AlmostEqual(expected), TStringBuilder() << actual << " " << expected);
    }
}

void TKinopoiskRecommendationsTest::Many() {
    constexpr size_t NUM_ROWS = 5000;

    UNIT_ASSERT(TableClient);

    const auto table = TScopedTable::Make<TYDBKinopoiskSVODRow>(
        *TableClient, NYdbHelpers::TTablePath{Db.Database, "kinopoisk"}, VIDEO_TABLE_KINOPOISK_PRIMARY_KEYS);

    TVector<TKinopoiskFilmInfo> expectedInfos;
    for (size_t i = 0; i < NUM_ROWS; ++i) {
        TKinopoiskFilmInfo info;
        info.Id = ToString(i);
        expectedInfos.push_back(std::move(info));
    }

    UNIT_ASSERT_VALUES_EQUAL(expectedInfos.size(), NUM_ROWS);

    {
        TTableWriter<TYDBKinopoiskSVODRow> writer{*TableClient, table.Path()};
        for (const auto& info : expectedInfos) {
            TYDBKinopoiskSVODRow row;
            info.Ser(row);
            writer.AddRow(row);
        }
    }

    Sort(expectedInfos, [&](const TKinopoiskFilmInfo& lhs, const TKinopoiskFilmInfo& rhs) { return lhs.Id < rhs.Id; });

    const auto status = GetFilmsInfos(*TableClient, table.Path());
    ThrowOnError(status);
    const auto& actualInfos = status.GetInfos();
    UNIT_ASSERT_VALUES_EQUAL(actualInfos.size(), NUM_ROWS);

    for (size_t i = 0; i < NUM_ROWS; ++i) {
        const auto& actual = actualInfos[i];
        const auto& expected = expectedInfos[i];
        UNIT_ASSERT_C(actual.AlmostEqual(expected), TStringBuilder() << actual << " " << expected);
    }
}

void TKinopoiskRecommendationsTest::BadData() {
    UNIT_ASSERT(TableClient);

    const auto table = TScopedTable::Make<TInvalidKinopoiskSVODRow>(
        *TableClient, NYdbHelpers::TTablePath{Db.Database, "kinopoisk"}, VIDEO_TABLE_KINOPOISK_PRIMARY_KEYS);

    {
        TTableWriter<TInvalidKinopoiskSVODRow> writer{*TableClient, table.Path()};

        TInvalidKinopoiskSVODRow row;
        row.SetKinopoiskId("000");
        row.SetFakeDoubleField(42);
        row.SetFakeStringField("trololo");

        writer.AddRow(row);
    }

    const auto status = GetFilmsInfos(*TableClient, table.Path());
    ThrowOnError(status);
}
} // namespace
