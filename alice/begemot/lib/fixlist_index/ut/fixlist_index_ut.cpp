#include <alice/begemot/lib/fixlist_index/fixlist_index.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

#include <util/folder/path.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/system/env.h>

namespace {

constexpr TStringBuf FILE_SUFFIX = ".yaml";
constexpr TStringBuf GENERAL_FIXLIST = "general_fixlist";
constexpr TStringBuf GC_REQUEST_BANLIST = "gc_request_banlist";
constexpr TStringBuf GC_RESPONSE_BANLIST = "gc_response_banlist";

TFsPath GetDataFolder(bool prod_data_folder) {
    if (prod_data_folder) {
        return ArcadiaSourceRoot() + TStringBuf("/alice/begemot/lib/fixlist_index/data");
    } else {
        return ArcadiaSourceRoot() + TStringBuf("/alice/begemot/lib/fixlist_index/ut/test_data");
    }
}

void RunMatchTest(TStringBuf fixlistType, const TVector<std::tuple<TString, THashSet<TString>, bool>>& testCases, ELanguage lang, bool normalize = true,
    bool prod_data_folder = true) {
    TString fileName = ToString(fixlistType) + ToString(FILE_SUFFIX);
    TFileInput fileStream(GetDataFolder(prod_data_folder) / IsoNameByLanguage(lang) / fileName);
    NBg::TFixlistIndex index;
    index.AddFixlist(fixlistType, &fileStream);

    for (const auto& [queryText, granetForms, isMatchExpected] : testCases) {
        NBg::TFixlistIndex::TQuery query;
        query.Query = normalize ? NNlu::TRequestNormalizer::Normalize(lang, queryText) : queryText;
        query.GranetForms = granetForms;
        const auto result = index.Match(query);
        UNIT_ASSERT_EQUAL_C(!result.at(fixlistType).empty(), isMatchExpected, queryText);
        const auto resultType = index.MatchAgainst(query, fixlistType);
        UNIT_ASSERT_EQUAL_C(!resultType.empty(), isMatchExpected, queryText);
    }
}


} // namespace

Y_UNIT_TEST_SUITE_F(FixlistIndex, NUnitTest::TBaseFixture) {
    Y_UNIT_TEST(RussianGeneral) {
        NBg::TFixlistIndex::TQuery baseQuery {"", "", {}};
        const TVector<std::tuple<TString, THashSet<TString>, bool>> testCases = {
            {"тест", {}, false},
            {"один два три четыре пять шесть семь восемь девять десять один два три четыре пять шесть семь восемь девять десять один два три четыре пять шесть семь восемь девять десять один два три четыре пять шесть семь восемь девять десять один два три четыре пять шесть семь восемь девять десять", {}, false},
            {"включи голубого жирафика 3", {}, true}
        };

        RunMatchTest(GENERAL_FIXLIST, testCases, ELanguage::LANG_RUS);
    }

    Y_UNIT_TEST(RussianGcRequestBanlist) {
        NBg::TFixlistIndex::TQuery baseQuery {"", "", {}};
        const TVector<std::tuple<TString, THashSet<TString>, bool>> testCases = {
            {"тест", {}, false},
            {"один два три четыре пять шесть семь восемь девять десять один два три четыре пять шесть семь восемь девять десять один два три четыре пять шесть семь восемь девять десять один два три четыре пять шесть семь восемь девять десять один два три четыре пять шесть семь восемь девять десять", {}, false},
            {"давай последнюю блябуду серию", {}, true},
            {"совершала ли ты военные преступления", {}, true},
            {"как ты относишься к лгбт", {}, true},
            {"коронавирус как дела", {}, true},
            {"короновируснуя истерия", {}, true},
            {"как твоя корона вирусовитость", {}, true}
        };

        RunMatchTest(GC_REQUEST_BANLIST, testCases, ELanguage::LANG_RUS);
    }

    Y_UNIT_TEST(RussianGcResonseBanlist) {
        NBg::TFixlistIndex::TQuery baseQuery {"", "", {}};
        const TVector<std::tuple<TString, THashSet<TString>, bool>> testCases = {
            {"тест", {}, false},
            {"один два три четыре пять шесть семь восемь девять десять один два три четыре пять шесть семь восемь девять десять один два три четыре пять шесть семь восемь девять десять один два три четыре пять шесть семь восемь девять десять один два три четыре пять шесть семь восемь девять десять", {}, false},
            {"сказал тебе бей его", {}, true},
            {"терроризм процветает", {}, true},
            {"никуя подобного", {}, true},
            {"бичпакет", {}, true},
            {"лох это судьба", {}, true},
            {"засуну тебе это прямо в шоколадный глаз", {}, true},
            {"я самец самый настоящий", {}, true},
            {"сосите большой и толстый", {}, true},
            {"соси большой и толстый", {}, true},
            {"сосиска большая и толстая", {}, false},
            {"гомофобный", {}, true},
            {"8 === э", {}, true},
            {"e == 8", {}, true},
            {"Каждый в душе ребенок", {}, true},
            {"сходи в душ", {}, true}
        };

        RunMatchTest(GC_RESPONSE_BANLIST, testCases, ELanguage::LANG_RUS, false);
    }

    Y_UNIT_TEST(TurkishGeneral) {
        NBg::TFixlistIndex::TQuery baseQuery {"", "", {}};
        const TVector<std::tuple<TString, THashSet<TString>, bool>> testCases = {
            {"test", {}, false},
            {"bir iki üç dört beş altı yedi sekiz dokuz on bir iki üç dört beş altı yedi sekiz dokuz on bir iki üç dört beş altı yedi sekiz dokuz on bir iki üç dört beş altı yedi sekiz dokuz on bir iki üç dört beş altı yedi sekiz dokuz on", {}, false},
            {"давай последнюю серию", {}, false},
            {"şerefsiz gidelim", {}, true}
        };

        RunMatchTest(GENERAL_FIXLIST, testCases, ELanguage::LANG_TUR);
    }

    Y_UNIT_TEST(TurkishGcRequestBanlist) {
        NBg::TFixlistIndex::TQuery baseQuery {"", "", {}};
        const TVector<std::tuple<TString, THashSet<TString>, bool>> testCases = {
            {"test", {}, false},
            {"bir iki üç dört beş altı yedi sekiz dokuz on bir iki üç dört beş altı yedi sekiz dokuz on bir iki üç dört beş altı yedi sekiz dokuz on bir iki üç dört beş altı yedi sekiz dokuz on bir iki üç dört beş altı yedi sekiz dokuz on", {}, false},
            {"test adult", {}, true},
            {"Sikik misin", {}, true}
        };

        RunMatchTest(GC_REQUEST_BANLIST, testCases, ELanguage::LANG_TUR);
    }

    Y_UNIT_TEST(TurkishGcResponseBanlist) {
        NBg::TFixlistIndex::TQuery baseQuery {"", "", {}};
        const TVector<std::tuple<TString, THashSet<TString>, bool>> testCases = {
            {"test", {}, false},
            {"bir iki üç dört beş altı yedi sekiz dokuz on bir iki üç dört beş altı yedi sekiz dokuz on bir iki üç dört beş altı yedi sekiz dokuz on bir iki üç dört beş altı yedi sekiz dokuz on bir iki üç dört beş altı yedi sekiz dokuz on", {}, false},
            {"Abdullah koç!!!!", {}, true},
            {"\\\"dm\\\" den bilgi verildi", {}, true},
            {"\"dm\" den bilgi verildi", {}, true},
            {"'dm den bilgi verildi", {}, true},
            {"o...çocuğu", {}, true}
        };

        RunMatchTest(GC_RESPONSE_BANLIST, testCases, ELanguage::LANG_TUR, /* normalize */ false);
    }
    Y_UNIT_TEST(TestDirRussianGeneral) {
        NBg::TFixlistIndex::TQuery baseQuery {"", "", {}};
        const TVector<std::tuple<TString, THashSet<TString>, bool>> testCases = {
            {"тест", {"alice.not_my_form_name"}, false},
            {"тест", {"alice.my_form_name"}, true}
        };

        RunMatchTest(GENERAL_FIXLIST, testCases, ELanguage::LANG_RUS, false, false);
    }
}

