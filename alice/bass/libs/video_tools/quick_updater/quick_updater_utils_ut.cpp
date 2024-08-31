#include "quick_updater_utils.h"

#include <alice/bass/libs/video_common/content_db.h>
#include <alice/bass/libs/video_common/video_ut_helpers.h>

#include <alice/bass/libs/ut_helpers/test_with_ydb.h>

#include <alice/bass/libs/video_common/providers.h>
#include <alice/bass/libs/video_content/common.h>
#include <alice/bass/libs/video_content/protos/rows.pb.h>
#include <alice/bass/libs/ydb_helpers/path.h>
#include <alice/bass/libs/ydb_helpers/table.h>
#include <alice/bass/libs/ydb_helpers/testing.h>

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/system/types.h>
#include <util/system/yassert.h>

using namespace NQuickUpdater;
using namespace NTestingHelpers;
using namespace NVideoCommon;
using namespace NYdbHelpers;

namespace {

class TQuickUpdaterTest : public NTesting::TestWithYDB {
public:
    TQuickUpdaterTest()
        : IviGenres{IviDelegate}
        , UAPIFactory{"" /* empty ott ticket */}
        , ProvidersCache{UAPIFactory, IviGenres, {"kinopoisk"}, TRPSConfig{}}
    {
    }

    void SetUp() override {
        NTesting::TestWithYDB::SetUp();
        OldDir = std::make_unique<TDbVideoDirectory>(*TableClient, *SchemeClient, GetOldDirTablePath());
        FillContentDb();
    }

    void TearDown() override {
        OldDir.reset();
        NTesting::TestWithYDB::TearDown();
    }

private:
    TTablePath GetOldDirTablePath() {
        return TTablePath{Db.Database, "video_20190514T120000"};
    }


    UNIT_TEST_SUITE(TQuickUpdaterTest);
    UNIT_TEST(GetSerialsForUpdate)
    UNIT_TEST(UpdateContentDbTvShows);
    UNIT_TEST_SUITE_END();

    void FillContentDb();
    void GetSerialsForUpdate();
    void UpdateContentDbTvShows();

private:
    std::unique_ptr<TDbVideoDirectory> OldDir;

    TIviGenres IviGenres;
    NTestingHelpers::TIviGenresDelegate IviDelegate;
    TUAPIDownloaderProviderFactory UAPIFactory;
    TContentInfoProvidersCache ProvidersCache;

};
UNIT_TEST_SUITE_REGISTRATION(TQuickUpdaterTest);

TVector<TTvShowAllInfo> MakeTvShowData() {
    TVector<TTvShowAllInfo> infos;
    {
        TVideoItem mashaAndTheBear;
        mashaAndTheBear->Description() = "Мультфильм Маша и Медведь";
        mashaAndTheBear->ProviderItemId() = "mb";
        mashaAndTheBear->HumanReadableId() = "masha-and-the-bear";
        mashaAndTheBear->ProviderName() = PROVIDER_KINOPOISK;
        mashaAndTheBear->SeasonsCount() = 2;
        mashaAndTheBear->Type() = "tv_show";

        TSerialDescriptor mashaSerial;
        mashaSerial.Id = mashaAndTheBear->ProviderItemId();
        TSeasonDescriptor mashaS1 =
            NTestingHelpers::MakeSeason(PROVIDER_KINOPOISK, mashaSerial.Id, "ms1" /* id */, 0, 4);
        TSeasonDescriptor mashaS2 =
            NTestingHelpers::MakeSeason(PROVIDER_KINOPOISK, mashaSerial.Id, "ms2" /* id */, 1, 3);
        mashaS2.UpdateAt = TInstant::ParseIso8601("2019-05-15T11:00:00Z");
        mashaSerial.Seasons.push_back(std::move(mashaS1));
        mashaSerial.Seasons.push_back(std::move(mashaS2));
        infos.push_back(TTvShowAllInfo{mashaAndTheBear, mashaSerial});
    }

    {
        TVideoItem breakingBad;
        breakingBad->Description() = "Во все тяжкие";
        breakingBad->ProviderItemId() = "bb";
        breakingBad->HumanReadableId() = "breaking-bad";
        breakingBad->MiscIds()->Kinopoisk() = "34567";
        breakingBad->ProviderName() = PROVIDER_KINOPOISK;
        breakingBad->Type() = "tv_show";

        TSerialDescriptor breakingBadSerial;
        breakingBadSerial.Id = breakingBad->ProviderItemId();
        TSeasonDescriptor breakingBadS1 =
            NTestingHelpers::MakeSeason(PROVIDER_KINOPOISK, breakingBadSerial.Id, "bs1" /* id */, 0, 5);
        TSeasonDescriptor breakingBadS2 =
            NTestingHelpers::MakeSeason(PROVIDER_KINOPOISK, breakingBadSerial.Id, "bs2" /* id */, 1, 6);
        breakingBadS2.UpdateAt = TInstant::ParseIso8601("2019-05-15T12:00:00Z");
        breakingBadSerial.Seasons.push_back(std::move(breakingBadS1));
        breakingBadSerial.Seasons.push_back(std::move(breakingBadS2));
        infos.push_back(TTvShowAllInfo{breakingBad, breakingBadSerial});
    }

    {
        TVideoItem gameOfThrones;
        gameOfThrones->Description() = "Игра престолов";
        gameOfThrones->ProviderItemId() = "got";
        gameOfThrones->HumanReadableId() = "game-of-thrones";
        gameOfThrones->MiscIds()->Kinopoisk() = "23456";
        gameOfThrones->ProviderName() = PROVIDER_KINOPOISK;
        gameOfThrones->Type() = "tv_show";
        gameOfThrones->UpdateAtUs() = TInstant::ParseIso8601("2019-05-15T11:30:00Z").MicroSeconds();

        TSerialDescriptor gameOfThronesSerial;
        gameOfThronesSerial.Id = gameOfThrones->ProviderItemId();
        TSeasonDescriptor gameOfThronesS1 =
            NTestingHelpers::MakeSeason(PROVIDER_KINOPOISK, gameOfThronesSerial.Id, "gs1" /* id */, 0, 1);
        gameOfThronesSerial.Seasons.push_back(std::move(gameOfThronesS1));

        infos.push_back(TTvShowAllInfo{gameOfThrones, gameOfThronesSerial});
    }

    return infos;
}

void TQuickUpdaterTest::FillContentDb() {
    Y_ASSERT(TableClient);

    TVideoItem theHatefulEight;
    theHatefulEight->Description() = "Фильм омерзительная восьмерка";
    theHatefulEight->ProviderItemId() = "123";
    theHatefulEight->HumanReadableId() = "the-hateful-eight";
    theHatefulEight->MiscIds()->Kinopoisk() = "86712";
    theHatefulEight->ProviderName() = PROVIDER_KINOPOISK;
    theHatefulEight->Type() = "movie";

    TVideoItem superHeroFilm;
    superHeroFilm->Description() = "Фильм про супергероев";
    superHeroFilm->ProviderItemId() = "superherofilm";
    superHeroFilm->HumanReadableId() = "superherofilm";
    superHeroFilm->MiscIds()->Kinopoisk() = "12345";
    superHeroFilm->ProviderName() = PROVIDER_KINOPOISK;
    superHeroFilm->Type() = "movie";

    auto tvShows = MakeTvShowData();
    Y_ASSERT(tvShows.size() == 3);
    const auto& mashaAndTheBear = tvShows[0];
    const auto& breakingBad = tvShows[1];
    const auto& gameOfThrones = tvShows[2];

    OldDir->WriteItems({theHatefulEight, superHeroFilm, mashaAndTheBear.Item, gameOfThrones.Item, breakingBad.Item});
    OldDir->WriteSerialDescriptor(mashaAndTheBear.SerialDescr.Descr, PROVIDER_KINOPOISK);
    OldDir->WriteSerialDescriptor(breakingBad.SerialDescr.Descr, PROVIDER_KINOPOISK);
    OldDir->WriteSerialDescriptor(gameOfThrones.SerialDescr.Descr, PROVIDER_KINOPOISK);
}

void TQuickUpdaterTest::GetSerialsForUpdate() {
    TTablePath itemsPath = NYdbHelpers::Join(OldDir->DirectoryPath, NVideoContent::TVideoItemsLatestTableTraits::NAME);
    TTablePath seasonsPath = NYdbHelpers::Join(OldDir->DirectoryPath, NVideoContent::TVideoSeasonsTableTraits::NAME);
    TInstant time10AM = TInstant::ParseIso8601("2019-05-15T10:00:00Z");
    TInstant time11AM = TInstant::ParseIso8601("2019-05-15T11:00:00Z");
    TInstant time12AM = TInstant::ParseIso8601("2019-05-15T12:00:00Z");

    TSerialsForUpdateStatus status10AM =
        NQuickUpdater::GetSerialsForUpdate(*TableClient, itemsPath, seasonsPath, time10AM);
    TSerialsForUpdateStatus status11AM =
        NQuickUpdater::GetSerialsForUpdate(*TableClient, itemsPath, seasonsPath, time11AM);
    TSerialsForUpdateStatus status12AM =
        NQuickUpdater::GetSerialsForUpdate(*TableClient, itemsPath, seasonsPath, time12AM);

    UNIT_ASSERT(status10AM.IsSuccess());
    UNIT_ASSERT(status11AM.IsSuccess());
    UNIT_ASSERT(status12AM.IsSuccess());

    UNIT_ASSERT(status10AM.GetItems().empty());
    UNIT_ASSERT_VALUES_EQUAL(status11AM.GetItems().size(), 1u);
    UNIT_ASSERT_VALUES_EQUAL(status12AM.GetItems().size(), 3u);
}

void TQuickUpdaterTest::UpdateContentDbTvShows() {
    TTablePath newSnapshotPath = TTablePath{Db.Database, "data_new"};

    {
        std::unique_ptr<TTransaction> copyTransaction;
        CopyTables(*TableClient, *SchemeClient, GetOldDirTablePath(), newSnapshotPath, copyTransaction);

        TYdbContentDb db{*TableClient, TVideoTablesPaths::MakeDefault(newSnapshotPath),
                         TIndexTablesPaths::MakeDefault(newSnapshotPath)};
        auto infos = MakeTvShowData();
        infos[0].SeasonDescrs[1].Descr.UpdateAt = {};
        infos[1].SeasonDescrs[1].Descr.UpdateAt = {};
        infos[2].SeasonDescrs[0].Descr.UpdateAt = {};
        (*infos[2].Item->GetRawValue())["update_at_us"].Clear();
        NQuickUpdater::UpdateContentDbTvShows(db, *TableClient, newSnapshotPath, infos);

        {
            TSeasonDescriptor foundSeason;
            const auto status = db.FindSeasonDescriptor(
                TSeasonKey{PROVIDER_KINOPOISK, infos[0].Item->ProviderItemId(), 2u}, foundSeason);
            UNIT_ASSERT(status.IsSuccess());
            UNIT_ASSERT(!foundSeason.UpdateAt);
        }
        {
            TSeasonDescriptor foundSeason;
            const auto status = db.FindSeasonDescriptor(
                TSeasonKey{PROVIDER_KINOPOISK, infos[0].Item->ProviderItemId(), 2u}, foundSeason);
            UNIT_ASSERT(status.IsSuccess());
            UNIT_ASSERT(!foundSeason.UpdateAt);
        }

        {
            TSeasonDescriptor foundSeason;
            const auto status = db.FindSeasonDescriptor(
                TSeasonKey{PROVIDER_KINOPOISK, infos[1].Item->ProviderItemId(), 1u}, foundSeason);
            UNIT_ASSERT(status.IsSuccess());
            UNIT_ASSERT(!foundSeason.UpdateAt);
        }

        {
            TVideoItem foundItem;
            const auto status =
                db.FindVideoItem(*TVideoKey::TryFromProviderItemId(
                                     PROVIDER_KINOPOISK, TVideoItemConstScheme{infos[2].Item->GetRawValue()}),
                                 foundItem);
            UNIT_ASSERT(status.IsSuccess());
            UNIT_ASSERT(!foundItem->HasUpdateAtUs());
        }

        {
            TVideoItem foundEpisode;
            const auto status = db.FindProviderUniqueVideoItem(
                TVideoKey{PROVIDER_KINOPOISK, "mb_s1e1", TVideoKey::EIdType::ID, ToString(EItemType::TvShowEpisode)},
                foundEpisode);
            UNIT_ASSERT(status.IsSuccess());
        }
    }

    bool isRolledBack = true;
    const auto status =
        ListDirectory(*SchemeClient, Db.Database, [&isRolledBack](const NYdb::NScheme::TSchemeEntry& entry) {
            if (entry.Name == "data_new")
                isRolledBack = false;
        });
    UNIT_ASSERT(status.IsSuccess());
    UNIT_ASSERT(isRolledBack);
}

} // namespace
