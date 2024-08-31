#include "content_parser.h"

#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/env.h>

#include <util/stream/file.h>
#include <util/folder/path.h>

namespace NAlice::NHollywoodFw::NMusic::NFmRadio {

namespace {

constexpr TStringBuf TEST_DATA_DIR = "alice/hollywood/library/scenarios/music/scene/fm_radio/ut/data";
constexpr TStringBuf FILENAME = "fm_radio.json";

TString ReadTestData(TStringBuf filename) {
    return TFileInput(TFsPath(ArcadiaSourceRoot()) / TEST_DATA_DIR / filename).ReadAll();
}

} // namespace

Y_UNIT_TEST_SUITE(FmRadioParserTest) {

Y_UNIT_TEST(Smoke) {
    auto json = JsonFromString(ReadTestData(FILENAME));

    // check parsing
    auto radios = TFmRadioList::ParseFromJson(json);
    UNIT_ASSERT_EQUAL(radios.size(), 4);

    // check alphabetical sort
    radios.SortAlphabetically();
    TVector<TString> actualTitles;
    for (const auto& radio : radios) {
        actualTitles.push_back(radio.GetTitle());
    }
    TVector<TString> expectedTitles{"AAAAAAAAAAA", "BBBBBBBBBBB", "CCCCCCCCCCC", "DDDDDDDDDDD"};
    UNIT_ASSERT_EQUAL(actualTitles, expectedTitles);

    // check score sort
    radios.SortByScoreDesc();
    TVector<double> actualScores;
    for (const auto& radio : radios) {
        actualScores.push_back(radio.GetFmRadioInfo().GetScore());
    }
    TVector<double> expectedScores{4.0, 3.0, 2.0, 1.0};
    UNIT_ASSERT_EQUAL(actualScores, expectedScores);
}

}

} // namespace NAlice::NHollywoodFw::NMusic::NFmRadio
