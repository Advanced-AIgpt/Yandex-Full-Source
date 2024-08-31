#include "content_db.h"
#include "content_db_check_utils.h"
#include "video_ut_helpers.h"

#include <alice/bass/libs/ut_helpers/test_with_ydb.h>
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
using namespace NVideoContent;
using namespace NYdbHelpers;

using NTestingHelpers::TDbVideoDirectory;

using TSchemeClient = NYdb::NScheme::TSchemeClient;
using TTableClient = NYdb::NTable::TTableClient;

namespace {

class TContentDBCheckTest : public NTesting::TestWithYDB {
public:
    static constexpr TStringBuf CANDIDATE_DIR_NAME = "candidate";
    static constexpr TStringBuf REFERENCE_DIR_NAME = "reference";

    TTablePath GetCandidateTablePath() {
        return TTablePath{Db.Database, TString{CANDIDATE_DIR_NAME}};
    }

    TTablePath GetReferenceTablePath() {
        return TTablePath{Db.Database, TString{REFERENCE_DIR_NAME}};
    }

    void SetUp() override {
        NTesting::TestWithYDB::SetUp();
        CandidateTables = std::make_unique<TDbVideoDirectory>(*TableClient, *SchemeClient, GetCandidateTablePath());
        ReferenceTables = std::make_unique<TDbVideoDirectory>(*TableClient, *SchemeClient, GetReferenceTablePath());

        FillContentDb();
    }

    void TearDown() override {
        CandidateTables.reset();
        ReferenceTables.reset();
        NTesting::TestWithYDB::TearDown();
    }

    UNIT_TEST_SUITE(TContentDBCheckTest);
    UNIT_TEST(CheckTotalCounts);
    UNIT_TEST(CheckProviderCounts);
    UNIT_TEST(CheckIndividualItems);
    UNIT_TEST_SUITE_END();

    void CheckTotalCounts();
    void CheckProviderCounts();
    void CheckIndividualItems();

    void FillContentDb();

private:
    std::unique_ptr<TDbVideoDirectory> CandidateTables;
    std::unique_ptr<TDbVideoDirectory> ReferenceTables;
};
UNIT_TEST_SUITE_REGISTRATION(TContentDBCheckTest);

TResult LoggedCheck(TDbChecker& checker, const TDbCheckList& checks) {
    const auto error = checker.CheckContentDb(checks);
    if (error)
        LOG(ERR) << error->Msg << Endl;
    return error;
}

void TContentDBCheckTest::FillContentDb() {
    Y_ASSERT(TableClient);

    TVideoItem theHatefulEight;
    theHatefulEight->Description() = "Фильм омерзительная восьмерка";
    theHatefulEight->ProviderItemId() = "123";
    theHatefulEight->HumanReadableId() = "the-hateful-eight";
    theHatefulEight->MiscIds()->Kinopoisk() = "86712";
    theHatefulEight->ProviderName() = PROVIDER_AMEDIATEKA;
    theHatefulEight->Type() = "movie";

    TVideoItem superHeroFilm;
    superHeroFilm->Description() = "Фильм про супергероев";
    superHeroFilm->ProviderItemId() = "superherofilm";
    superHeroFilm->HumanReadableId() = "superherofilm";
    superHeroFilm->MiscIds()->Kinopoisk() = "12345";
    superHeroFilm->ProviderName() = PROVIDER_IVI;
    superHeroFilm->Type() = "movie";

    TVideoItem mashaAndTheBear;
    mashaAndTheBear->Description() = "Мультфильм Маша и Медведь";
    mashaAndTheBear->ProviderItemId() = "456";
    mashaAndTheBear->HumanReadableId() = "masha-and-the-bear";
    mashaAndTheBear->ProviderName() = PROVIDER_IVI;
    mashaAndTheBear->SeasonsCount() = 2;
    mashaAndTheBear->Type() = "tv_show";

    TVideoItem gameOfThrones;
    gameOfThrones->Description() = "Игра престолов";
    gameOfThrones->ProviderItemId() = "got";
    gameOfThrones->HumanReadableId() = "game-of-thrones";
    gameOfThrones->MiscIds()->Kinopoisk() = "23456";
    gameOfThrones->ProviderName() = PROVIDER_AMEDIATEKA;
    gameOfThrones->Type() = "tv_show";

    TVideoItem breakingBad;
    breakingBad->Description() = "Во все тяжкие";
    breakingBad->ProviderItemId() = "bb";
    breakingBad->HumanReadableId() = "breaking-bad";
    breakingBad->MiscIds()->Kinopoisk() = "34567";
    breakingBad->ProviderName() = PROVIDER_AMEDIATEKA;
    breakingBad->Type() = "tv_show";

    const TVector<TVideoItem> refItems = {theHatefulEight, superHeroFilm, mashaAndTheBear, gameOfThrones, breakingBad};
    const TVector<TVideoItem> candItems = {theHatefulEight, superHeroFilm, mashaAndTheBear, breakingBad};

    TSerialDescriptor mashaSerial;
    mashaSerial.Id = mashaAndTheBear->ProviderItemId();
    TSeasonDescriptor mashaS1 = NTestingHelpers::MakeSeason(PROVIDER_IVI, mashaSerial.Id, "s1" /* id */, 0, 4);
    TSeasonDescriptor mashaS2 = NTestingHelpers::MakeSeason(PROVIDER_IVI, mashaSerial.Id, "s2" /* id */, 1, 3);
    mashaSerial.Seasons.push_back(std::move(mashaS1));
    mashaSerial.Seasons.push_back(std::move(mashaS2));

    ReferenceTables->WriteItems(refItems);
    CandidateTables->WriteItems(candItems);
    CandidateTables->WriteSerialDescriptor(mashaSerial, PROVIDER_IVI);
}

void TContentDBCheckTest::CheckTotalCounts() {
    TDbCheckList okItemCount = ParseCheckListFromJsonOrThrow(R"({"total_amounts": {"items": {"expected_count": 4}}})");
    TDbCheckList failItemCount =
        ParseCheckListFromJsonOrThrow(R"({"total_amounts": {"items": {"expected_count": 5}}})");
    TDbCheckList okItemPerc =
        ParseCheckListFromJsonOrThrow(R"({"total_amounts": {"items": {"percentage_diff": 20}}})");
    TDbCheckList failItemPerc =
        ParseCheckListFromJsonOrThrow(R"({"total_amounts": {"items": {"percentage_diff": 15}}})");

    TDbCheckList okSerialsCount =
        ParseCheckListFromJsonOrThrow(R"({"total_amounts": {"serials": {"expected_count": 1}}})");
    TDbCheckList failSerialsCount =
        ParseCheckListFromJsonOrThrow(R"({"total_amounts": {"serials": {"expected_count": 2}}})");

    TDbCheckList okSeasonsCount =
        ParseCheckListFromJsonOrThrow(R"({"total_amounts": {"seasons": {"expected_count": 2}}})");
    TDbCheckList failSeasonsCount =
        ParseCheckListFromJsonOrThrow(R"({"total_amounts": {"seasons": {"expected_count": 3}}})");

    TDbChecker dbChecker{*TableClient, Db.Database, TString{CANDIDATE_DIR_NAME}, TString{REFERENCE_DIR_NAME}};

    UNIT_ASSERT(!LoggedCheck(dbChecker, okItemCount));
    UNIT_ASSERT(LoggedCheck(dbChecker, failItemCount));
    UNIT_ASSERT(!LoggedCheck(dbChecker, okItemPerc));
    UNIT_ASSERT(LoggedCheck(dbChecker, failItemPerc));

    UNIT_ASSERT(!LoggedCheck(dbChecker, okSerialsCount));
    UNIT_ASSERT(LoggedCheck(dbChecker, failSerialsCount));

    UNIT_ASSERT(!LoggedCheck(dbChecker, okSeasonsCount));
    UNIT_ASSERT(LoggedCheck(dbChecker, failSeasonsCount));
}

void TContentDBCheckTest::CheckProviderCounts() {
    TDbCheckList okItemCount =
        ParseCheckListFromJsonOrThrow(R"({"provider_amounts": {"ivi": {"items": {"expected_count": 2}}}})");
    TDbCheckList failItemCount =
        ParseCheckListFromJsonOrThrow(R"({"provider_amounts": {"amediateka": {"items": {"expected_count": 3}}}})");
    TDbCheckList okItemPerc =
        ParseCheckListFromJsonOrThrow(R"({"provider_amounts": {"ivi": {"items": {"percentage_diff": 10}}}})");
    TDbCheckList failItemPerc =
        ParseCheckListFromJsonOrThrow(R"({"provider_amounts": {"amediateka": {"items": {"percentage_diff": 30}}}})");

    TDbCheckList okSerialsCount =
        ParseCheckListFromJsonOrThrow(R"({"provider_amounts": {"ivi": {"serials": {"expected_count": 1}}}})");
    TDbCheckList failSerialsCount =
        ParseCheckListFromJsonOrThrow(R"({"provider_amounts": {"amediateka": {"serials": {"expected_count": 1}}}})");

    TDbCheckList okSeasonsCount =
        ParseCheckListFromJsonOrThrow(R"({"provider_amounts": {"ivi": {"seasons": {"expected_count": 2}}}})");
    TDbCheckList failSeasonsCount =
        ParseCheckListFromJsonOrThrow(R"({"provider_amounts": {"amediateka": {"seasons": {"expected_count": 1}}}})");

    TDbChecker dbChecker{*TableClient, Db.Database, TString{CANDIDATE_DIR_NAME}, TString{REFERENCE_DIR_NAME}};

    UNIT_ASSERT(!LoggedCheck(dbChecker, okItemCount));
    UNIT_ASSERT(LoggedCheck(dbChecker, failItemCount));
    UNIT_ASSERT(!LoggedCheck(dbChecker, okItemPerc));
    UNIT_ASSERT(LoggedCheck(dbChecker, failItemPerc));

    UNIT_ASSERT(!LoggedCheck(dbChecker, okSerialsCount));
    UNIT_ASSERT(LoggedCheck(dbChecker, failSerialsCount));

    UNIT_ASSERT(!LoggedCheck(dbChecker, okSeasonsCount));
    UNIT_ASSERT(LoggedCheck(dbChecker, failSeasonsCount));
}

void TContentDBCheckTest::CheckIndividualItems() {
    const TStringBuf okItemCheckJson = R"(
    {
      "single_items": [{
        "item": {
          "provider_item_id": "456",
          "provider_name": "ivi",
          "type": "tv_show",
          "seasons_count": 2
        },
        "serial_descriptor": {
          "seasons_count": 2,
          "seasons": [{
            "episodes_count": 4,
            "provider_number": 1
          }]
        }
      }]
    })";

    const TStringBuf noProviderNumberJson = R"(
    {
      "single_items": [{
        "item": {
          "provider_item_id": "456",
          "provider_name": "ivi",
          "type": "tv_show",
          "seasons_count": 2
        },
        "serial_descriptor": {
          "seasons_count": 2,
          "seasons": [{
            "episodes_count": 4
          }]
        }
      }]
    })";

    const TStringBuf nonExistingItemJson = R"(
    {
      "single_items": [{
        "item": {
          "provider_item_id": "got",
          "type": "tv_show",
          "provider_name": "amediateka"
        }
      }]
    })";

    const TStringBuf nonExistingSeasonJson = R"(
    {
      "single_items": [{
        "item": {
          "provider_item_id": "456",
          "provider_name": "ivi",
          "type": "tv_show",
          "seasons_count": 2
        },
        "serial_descriptor": {
          "seasons_count": 1,
          "seasons": [{
            "provider_number": 3
          }]
        }
      }]
    })";

    const TStringBuf episodesCountMismatchJson = R"(
    {
      "single_items": [{
        "item": {
          "provider_item_id": "456",
          "provider_name": "ivi",
          "type": "tv_show",
          "seasons_count": 2
        },
        "serial_descriptor": {
          "seasons_count": 2,
          "seasons": [{
            "episodes_count": 5,
            "provider_number": 1
          }]
        }
      }]
    })";

    UNIT_ASSERT_EXCEPTION_CONTAINS(ParseCheckListFromJsonOrThrow(noProviderNumberJson), yexception,
                                   "Failed to validate");
    TDbCheckList okItem = ParseCheckListFromJsonOrThrow(okItemCheckJson);
    TDbCheckList nonExistingItem = ParseCheckListFromJsonOrThrow(nonExistingItemJson);
    TDbCheckList nonExistingSeason = ParseCheckListFromJsonOrThrow(nonExistingSeasonJson);
    TDbCheckList episodesCountMismatch = ParseCheckListFromJsonOrThrow(episodesCountMismatchJson);

    TDbChecker dbChecker{*TableClient, Db.Database, TString{CANDIDATE_DIR_NAME}, TString{REFERENCE_DIR_NAME}};

    UNIT_ASSERT(!LoggedCheck(dbChecker, okItem));
    UNIT_ASSERT(LoggedCheck(dbChecker, nonExistingItem));
    UNIT_ASSERT(LoggedCheck(dbChecker, nonExistingSeason));
    UNIT_ASSERT(LoggedCheck(dbChecker, episodesCountMismatch));
}

} // namespace
