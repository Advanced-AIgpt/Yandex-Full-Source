#include "content_db.h"

#include "defs.h"
#include "video_ut_helpers.h"

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

using namespace NVideoCommon;
using namespace NYdbHelpers;
using NTestingHelpers::MakeSeason;

namespace {

constexpr TStringBuf IGRA_PRIESTOLOV = "igra-priestolov";
constexpr TStringBuf MASHA_I_MEDVED = "masha-i-medved";

class TContentDBTest : public NUnitTest::TTestBase {
public:
    void SetUp() override {
        Db.Init();
        Y_ASSERT(Db.Driver);

        TableClient = std::make_unique<NYdb::NTable::TTableClient>(*Db.Driver);

        Tables = TVideoTablesPaths::MakeDefault(Db.Database);
        Tables.ForEachPath([this](const TTablePath& path) { DropTable(*TableClient, path); });

        Indexes = TIndexTablesPaths::MakeDefault(Db.Database);
        Indexes.ForEachPath([this](const TTablePath& path) { DropTable(*TableClient, path); });
    }

    void TearDown() override {
        TableClient.reset();
    }

    UNIT_TEST_SUITE(TContentDBTest);
    UNIT_TEST(FindItems);
    UNIT_TEST(FindItemsList);
    UNIT_TEST(FindSerials);
    UNIT_TEST_SUITE_END();

    void FindItems();
    void FindItemsList();
    void FindSerials();

private:
    TLocalDatabase Db;
    std::unique_ptr<NYdb::NTable::TTableClient> TableClient;
    TVideoTablesPaths Tables;
    TIndexTablesPaths Indexes;
};
UNIT_TEST_SUITE_REGISTRATION(TContentDBTest);

void TContentDBTest::FindItemsList() {
    Y_ASSERT(TableClient);
    auto& client = *TableClient;
    const auto kpidsPath = Indexes.KpidsPath();
    const auto itemsPath = Tables.ItemsPath();

    UNIT_ASSERT(!CheckTableExists(client, kpidsPath).IsSuccess());
    UNIT_ASSERT(!CheckTableExists(client, itemsPath).IsSuccess());

    auto createVideoItem = [](TStringBuf kpid) -> TVideoItem {
        TVideoItem videoItem;
        videoItem->MiscIds()->Kinopoisk() = kpid;
        videoItem->Description() = "none";
        videoItem->ProviderItemId() = "none";
        videoItem->HumanReadableId() = "none";
        videoItem->ProviderName() = PROVIDER_KINOPOISK;
        videoItem->Type() = "movie";
        return videoItem;
    };

    TVector<TVideoItem> videoItemsInDb = {
        createVideoItem("0"),
        createVideoItem("1"),
        createVideoItem("2"),
        createVideoItem("3"),
        createVideoItem("4"),
    };
    TVector<TVideoItem> videoItemsNotInDb = {
        createVideoItem("00"),
        createVideoItem("11"),
        createVideoItem("22"),
        createVideoItem("33"),
        createVideoItem("44"),
    };

    CreateTableOrFail<NVideoContent::TVideoItemsLatestTableTraits>(client, itemsPath);
    CreateTableOrFail<NVideoContent::TKinopoiskIdIndexTableTraits>(client, kpidsPath);
    // Fill tables.
    {
        TTableWriter<NVideoContent::TVideoItemsLatestTableTraits::TScheme> itemsWriter{client, itemsPath};
        TTableWriter<NVideoContent::NProtos::TKinopoiskIdIndexRow> kpidsWriter{client, kpidsPath};

        for (ui64 id = 0; id < videoItemsInDb.size(); id++) {
            const auto& item = videoItemsInDb[id];
            {
                NVideoContent::TVideoItemsLatestTableTraits::TScheme row;
                Ser(item, id, false /*isVoid*/, row);
                itemsWriter.AddRow(row);
            }
            {
                NVideoContent::NProtos::TKinopoiskIdIndexRow row;
                if (Ser(item, id, row))
                    kpidsWriter.AddRow(row);
            }
        }
    }

    TYdbContentDb db{client, Tables, Indexes};
    // Start tests.
    auto checkRequest = [&db](const TVector<TVideoItem>& requestItems, const TVector<TVideoItem>& needResultItems) -> void {
        THashSet<TString> kpids;
        for (const auto& item : requestItems)
            kpids.insert(TString{*item->MiscIds()->Kinopoisk()});

        THashMap<TString, TVector<TVideoItem>> resultItemsMap = db.FindVideoItemsByKinopoiskIds(kpids);
        TVector<TVideoItem> resultItems;
        for (auto& items : resultItemsMap) {
            for (auto& item : items.second) {
                resultItems.push_back(std::move(item));
            }
        }

        bool isResultCorrect = resultItems.size() == needResultItems.size();
        for (const auto& needItem : needResultItems) {
            bool haveNeedItemInResult = FindIf(resultItems, [&needItem](const TVideoItem& x) {
                                            return NSc::TValue::Equal(x.Value(), needItem.Value());
                                        }) != resultItems.end();
            if (!isResultCorrect || !haveNeedItemInResult) {
                isResultCorrect = false;
                break;
            }
        }
        UNIT_ASSERT(isResultCorrect);
    };
    {
        const auto& requestItems = videoItemsInDb;
        const auto& needResultItems = videoItemsInDb;
        checkRequest(requestItems, needResultItems);
    }
    {
        const TVector<TVideoItem> requestItems = {videoItemsInDb[2], videoItemsInDb[1], videoItemsInDb[4], videoItemsInDb[0], videoItemsInDb[3]};
        const auto& needResultItems = requestItems;
        checkRequest(requestItems, needResultItems);
    }
    {
        TVector<TVideoItem> requestItems = {videoItemsInDb[4], videoItemsInDb[1]};
        TVector<TVideoItem> needResultItems = {videoItemsInDb[4], videoItemsInDb[1]};
        checkRequest(requestItems, needResultItems);
    }
    {
        const TVector<TVideoItem> requestItems = {videoItemsNotInDb[0], videoItemsNotInDb[1], videoItemsNotInDb[3]};
        const TVector<TVideoItem> needResultItems = {};
        checkRequest(requestItems, needResultItems);
    }
    {
        const TVector<TVideoItem> requestItems = {videoItemsNotInDb[4], videoItemsInDb[2], videoItemsNotInDb[3], videoItemsInDb[3]};
        const TVector<TVideoItem> needResultItems = {videoItemsInDb[2], videoItemsInDb[3]};
        checkRequest(requestItems, needResultItems);
    }
    {
        const TVector<TVideoItem> requestItems = {videoItemsNotInDb[4], videoItemsInDb[0], videoItemsInDb[1], videoItemsNotInDb[3]};
        const TVector<TVideoItem> needResultItems = {videoItemsInDb[0], videoItemsInDb[1]};
        checkRequest(requestItems, needResultItems);
    }
    {
        const TVector<TVideoItem> requestItems = {videoItemsInDb[1], videoItemsInDb[1], videoItemsInDb[1]};
        const TVector<TVideoItem> needResultItems = {videoItemsInDb[1]};
        checkRequest(requestItems, needResultItems);
    }
}

void TContentDBTest::FindItems() {
    Y_ASSERT(TableClient);
    auto& client = *TableClient;
    const auto itemsPath = Tables.ItemsPath();
    const auto providerUniqItemsPath = Tables.ProviderUniqueItemsPath();

    UNIT_ASSERT(!CheckTableExists(client, itemsPath).IsSuccess());

    CreateTableOrFail<NVideoContent::TVideoItemsLatestTableTraits>(client, itemsPath);
    CreateTableOrFail<NVideoContent::TProviderUniqueItemsTableTraitsV2>(client, providerUniqItemsPath);
    CreateTableOrFail<NVideoContent::TProviderItemIdIndexTableTraits>(client, Indexes.PiidsPath());
    CreateTableOrFail<NVideoContent::THumanReadableIdIndexTableTraits>(client, Indexes.HridsPath());
    CreateTableOrFail<NVideoContent::TKinopoiskIdIndexTableTraits>(client, Indexes.KpidsPath());

    TVideoItem theHatefulEight;
    theHatefulEight->Description() = "Фильм омерзительная восьмерка";
    theHatefulEight->ProviderItemId() = "123";
    theHatefulEight->HumanReadableId() = "the-hateful-eight";
    theHatefulEight->MiscIds()->Kinopoisk() = "86712";
    theHatefulEight->ProviderName() = PROVIDER_AMEDIATEKA;
    theHatefulEight->Type() = "movie";

    TVideoItem mashaAndTheBear;
    mashaAndTheBear->Description() = "Мультфильм Маша и Медведь";
    mashaAndTheBear->ProviderItemId() = "456";
    mashaAndTheBear->HumanReadableId() = "masha-and-the-bear";
    mashaAndTheBear->ProviderName() = PROVIDER_IVI;
    mashaAndTheBear->Type() = "tv_show";

    {
        TTableWriter<NVideoContent::NProtos::TVideoItemRowV5YDb> itemsWriter{client, itemsPath};
        TTableWriter<NVideoContent::NProtos::TProviderItemIdIndexRow> piidsWriter{client, Indexes.PiidsPath()};
        TTableWriter<NVideoContent::NProtos::THumanReadableIdIndexRow> hridsWriter{client, Indexes.HridsPath()};
        TTableWriter<NVideoContent::NProtos::TKinopoiskIdIndexRow> kpidsWriter{client, Indexes.KpidsPath()};
        TTableWriter<NVideoContent::NProtos::TProviderUniqueVideoItemRow> providerUniqItemsWriter{
            client, providerUniqItemsPath};

        const TVideoItem items[] = {theHatefulEight, mashaAndTheBear};
        for (ui64 id = 0; id < Y_ARRAY_SIZE(items); ++id) {
            const auto& item = items[id];
            {
                NVideoContent::NProtos::TVideoItemRowV5YDb row;
                Ser(item, id, false /* isVoid */, row);
                itemsWriter.AddRow(row);
            }

            {
                NVideoContent::NProtos::TProviderItemIdIndexRow row;
                if (Ser(item, id, row))
                    piidsWriter.AddRow(row);
            }
            {
                NVideoContent::NProtos::THumanReadableIdIndexRow row;
                if (Ser(item, id, row))
                    hridsWriter.AddRow(row);
            }
            {
                NVideoContent::NProtos::TKinopoiskIdIndexRow row;
                if (Ser(item, id, row))
                    kpidsWriter.AddRow(row);
            }

            {
                NVideoContent::NProtos::TProviderUniqueVideoItemRow row;
                if (Ser(item, row))
                    providerUniqItemsWriter.AddRow(row);
            }
        }
    }

    TYdbContentDb db{client, Tables, Indexes};

    {
        TVideoItem item;
        const auto status = db.FindVideoItem(
            TVideoKey{PROVIDER_IVI, "456" /* id */, TVideoKey::EIdType::ID, Nothing() /* type */}, item);
        UNIT_ASSERT_C(status.IsSuccess(), status);
        UNIT_ASSERT(NSc::TValue::Equal(mashaAndTheBear.Value(), item.Value()));
    }

    {
        TVideoItem item;
        const auto status = db.FindVideoItem(
            TVideoKey{PROVIDER_IVI, "456" /* id */, TVideoKey::EIdType::ID, "movie" /* type */}, item);
        UNIT_ASSERT_C(!status.IsSuccess(), status);
        UNIT_ASSERT(status.YdbStatus.IsSuccess());
        UNIT_ASSERT_VALUES_EQUAL(status.Status, TYdbContentDb::EStatus::NOT_FOUND);
    }

    {
        TVideoItem item;
        const auto status = db.FindVideoItem(TVideoKey{PROVIDER_AMEDIATEKA, "the-hateful-eight" /* id */,
                                                       TVideoKey::EIdType::HRID, Nothing() /* type */},
                                             item);
        UNIT_ASSERT_C(status.IsSuccess(), status);
        UNIT_ASSERT(NSc::TValue::Equal(theHatefulEight.Value(), item.Value()));
    }

    {
        TVideoItem item;
        const auto status = db.FindVideoItem(
            TVideoKey{PROVIDER_KINOPOISK, "kasl-rock" /* id */, TVideoKey::EIdType::HRID, Nothing() /* type */}, item);
        UNIT_ASSERT_C(!status.IsSuccess(), status);
        UNIT_ASSERT_VALUES_EQUAL(status.Status, TYdbContentDb::EStatus::NOT_FOUND);
    }

    {
        TVideoItem item;
        const auto status = db.FindVideoItem(
            TVideoKey{PROVIDER_AMEDIATEKA, "86712" /* id */, TVideoKey::EIdType::KINOPOISK_ID, "movie" /* type */},
            item);
        UNIT_ASSERT_C(status.IsSuccess(), status);
        UNIT_ASSERT(NSc::TValue::Equal(theHatefulEight.Value(), item.Value()));
    }

    {
        TVideoItem item;
        const auto status = db.FindVideoItem(
            TVideoKey{PROVIDER_AMEDIATEKA, "86712" /* id */, TVideoKey::EIdType::KINOPOISK_ID, "tv_show" /* type */},
            item);
        UNIT_ASSERT_C(!status.IsSuccess(), status);
        UNIT_ASSERT_VALUES_EQUAL(status.Status, TYdbContentDb::EStatus::NOT_FOUND);
    }

    {
        const auto status = db.FindId(TVideoKey{PROVIDER_AMEDIATEKA, "masha-and-the-bear", TVideoKey::EIdType::HRID,
                                                ToString(EItemType::TvShow)});
        UNIT_ASSERT_C(status.IsSuccess(), status);
        UNIT_ASSERT_VALUES_EQUAL(status.Id, Nothing());
    }

    {
        const auto status = db.FindId(TVideoKey{PROVIDER_IVI, "masha-and-the-bear", TVideoKey::EIdType::HRID,
                                                ToString(EItemType::TvShow)});
        UNIT_ASSERT_C(status.IsSuccess(), status);
        UNIT_ASSERT_VALUES_EQUAL(status.Id, static_cast<ui64>(1));
    }

    {
        const auto key = TVideoKey::TryAny(theHatefulEight->ProviderName(), theHatefulEight.Scheme());
        UNIT_ASSERT(key.Defined());
        const auto status = db.FindId(*key);
        UNIT_ASSERT_C(status.IsSuccess(), status);
        UNIT_ASSERT_VALUES_EQUAL(status.Id, static_cast<ui64>(0));
    }

    {
        const auto key = TVideoKey::TryAny(mashaAndTheBear->ProviderName(), mashaAndTheBear.Scheme());
        UNIT_ASSERT(key.Defined());
        const auto status = db.FindId(*key);
        UNIT_ASSERT_C(status.IsSuccess(), status);
        UNIT_ASSERT_VALUES_EQUAL(status.Id, static_cast<ui64>(1));
    }

    {
        TVideoItem episodeDb;
        episodeDb->Description() = "Episode";
        episodeDb->ProviderItemId() = "s1e1";
        episodeDb->HumanReadableId() = "episodeS1E1";
        episodeDb->ProviderName() = PROVIDER_KINOPOISK;
        episodeDb->Type() = "tv_show_episode";

        {
            TTableWriter<NVideoContent::NProtos::TProviderUniqueVideoItemRow> providerUniqItemsWriter{
                client, providerUniqItemsPath};
            NVideoContent::NProtos::TProviderUniqueVideoItemRow row;
            if (Ser(episodeDb, row))
                providerUniqItemsWriter.AddRow(row);
        }

        {
            const auto key =
                TVideoKey::TryFromProviderItemId(mashaAndTheBear->ProviderName(), mashaAndTheBear.Scheme());
            UNIT_ASSERT(key.Defined());
            TVideoItem item;
            const auto status = db.FindProviderUniqueVideoItem(*key, item);
            UNIT_ASSERT_C(status.IsSuccess(), status);
            UNIT_ASSERT(NSc::TValue::Equal(mashaAndTheBear.Value(), item.Value()));
        }

        {
            const auto key = TVideoKey::TryFromProviderItemId(PROVIDER_KINOPOISK, episodeDb.Scheme());
            UNIT_ASSERT(key.Defined());

            TVideoItem episode;
            const auto status = db.FindProviderUniqueVideoItem(*key, episode);
            UNIT_ASSERT_C(status.IsSuccess(), status);
            UNIT_ASSERT_VALUES_EQUAL(episode->Type(), ToString(EItemType::TvShowEpisode));

            const auto itemsStatus = db.FindVideoItem(*key, episode);
            UNIT_ASSERT_C(!itemsStatus.IsSuccess(), status);
            UNIT_ASSERT_VALUES_EQUAL(itemsStatus.Status, TYdbContentDb::EStatus::NOT_FOUND);
        }
    }
}

void TContentDBTest::FindSerials() {
    Y_ASSERT(TableClient);
    auto& client = *TableClient;
    const auto serialsPath = Tables.SerialsPath();
    const auto seasonsPath = Tables.SeasonsPath();

    UNIT_ASSERT(!CheckTableExists(client, serialsPath).IsSuccess());

    CreateTableOrFail<NVideoContent::TVideoSerialsTableTraits>(client, serialsPath);
    CreateTableOrFail<NVideoContent::TVideoSeasonsTableTraits>(client, seasonsPath);

    const auto season1 = MakeSeason(PROVIDER_AMEDIATEKA, IGRA_PRIESTOLOV, Nothing() /* id */, 0 /* seasonIndex */,
                                    2 /* episodesCount */);
    const auto season2 = MakeSeason(PROVIDER_AMEDIATEKA, IGRA_PRIESTOLOV, Nothing() /* id */, 1 /* seasonIndex */,
                                    3 /* episodesCount */);
    const auto season5 = MakeSeason(PROVIDER_AMEDIATEKA, IGRA_PRIESTOLOV, Nothing() /* id */, 4 /* seasonIndex */,
                                    10 /* episodesCount */);

    TSerialDescriptor serial;
    {
        serial.Id = IGRA_PRIESTOLOV;

        for (const auto& season : {season1, season2, season5}) {
            serial.Seasons.push_back(season);
            serial.TotalEpisodesCount += season.EpisodesCount;
        }
    }

    {
        TTableWriter<NVideoContent::NProtos::TSerialDescriptorRow> writer(client, serialsPath);
        NVideoContent::NProtos::TSerialDescriptorRow row;
        UNIT_ASSERT(serial.Ser(TSerialKey{PROVIDER_AMEDIATEKA, serial.Id}, row));
        writer.AddRow(row);
    }

    {
        TTableWriter<NVideoContent::NProtos::TSeasonDescriptorRow> writer(client, seasonsPath);
        for (const auto& season : {season1, season2, season5}) {
            NVideoContent::NProtos::TSeasonDescriptorRow row;
            UNIT_ASSERT(season.Ser(TSeasonKey{PROVIDER_AMEDIATEKA, season.SerialId, season.ProviderNumber}, row));
            writer.AddRow(row);
        }
    }

    TYdbContentDb db{client, Tables, Indexes};
    {
        TSerialDescriptor actual;
        const auto status = db.FindSerialDescriptor(TSerialKey{PROVIDER_AMEDIATEKA, IGRA_PRIESTOLOV}, actual);

        UNIT_ASSERT_C(status.IsSuccess(), status);
        UNIT_ASSERT_VALUES_EQUAL(serial, actual);
    }

    {
        TSerialDescriptor actual;
        const auto status = db.FindSerialDescriptor(TSerialKey{PROVIDER_IVI, MASHA_I_MEDVED}, actual);

        UNIT_ASSERT_C(!status.IsSuccess(), status);
        UNIT_ASSERT_VALUES_EQUAL(status.Status, TYdbContentDb::EStatus::NOT_FOUND);
    }

    {
        TSeasonDescriptor actual;
        const auto status =
            db.FindSeasonDescriptor(TSeasonKey{PROVIDER_AMEDIATEKA, IGRA_PRIESTOLOV, 5 /* providerNumber */}, actual);
        UNIT_ASSERT_C(status.IsSuccess(), status);
        UNIT_ASSERT_VALUES_EQUAL(season5, actual);
    }

    {
        TSeasonDescriptor actual;
        const auto status =
            db.FindSeasonDescriptor(TSeasonKey{PROVIDER_AMEDIATEKA, IGRA_PRIESTOLOV, 2 /* providerNumber */}, actual);
        UNIT_ASSERT_C(status.IsSuccess(), status);
        UNIT_ASSERT_VALUES_EQUAL(season2, actual);
    }

    {
        TSeasonDescriptor actual;
        const auto status =
            db.FindSeasonDescriptor(TSeasonKey{PROVIDER_AMEDIATEKA, IGRA_PRIESTOLOV, 3 /* providerNumber */}, actual);
        UNIT_ASSERT_C(!status.IsSuccess(), status);
        UNIT_ASSERT_VALUES_EQUAL(status.Status, TYdbContentDb::EStatus::NOT_FOUND);
    }
}

} // namespace
