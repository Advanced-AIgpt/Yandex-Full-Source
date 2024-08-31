#include "lemmatize.h"

#include <alice/nlu/libs/ut_utils/ut_utils.h>
#include <library/cpp/testing/unittest/registar.h>

#include <utility>

using namespace NNlu;

Y_UNIT_TEST_SUITE(Lemmatize) {

    void TestLemmatizeWord(ELanguage lang, double threshold,
        const TVector<std::pair<TStringBuf, TVector<TStringBuf>>>& data)
    {
        for (const auto& [word, expected] : data) {
            NAlice::NUtUtils::TestEqualSeq(word, expected, LemmatizeWord(word, lang, threshold));
        }
    }

    void TestLemmatizeWordBest(ELanguage lang, const TVector<std::pair<TStringBuf, TStringBuf>>& data) {
        for (const auto& [word, expected] : data) {
            NAlice::NUtUtils::TestEqual(word, expected, LemmatizeWordBest(word, lang));
        }
    }

    Y_UNIT_TEST(LemmatizeWordRu) {
        TestLemmatizeWord(LANG_RUS, ANY_LEMMA_THRESHOLD, {
            {"Включи",      {"включать"}},
            {"Включай",     {"включать"}},
            {"Включить",    {"включать"}},
            {"Моё",         {"мой"}},
            {"Мне",         {"я"}},
            {"ели",         {"есть", "ель"}},
            {"ёлку",        {"елка"}},
            {"Елку",        {"елка"}},
            {"ёлочку",      {"елочка"}},
            {"123",         {"123"}},
            {"АИ92",        {"аи92"}},
            {"всё",         {"весь", "все"}},
            {"всe",         {"весь", "все"}},
            {"этого",       {"это", "этот"}},
            {"сорока",      {"сорок", "сорока"}},
        });
    }

    Y_UNIT_TEST(LemmatizeWordTr) {
        TestLemmatizeWord(LANG_TUR, ANY_LEMMA_THRESHOLD, {
            {"kamerası",    {"kamera"}},
            {"Kamerası",    {"kamera"}},
            {"KAMERASI",    {"kamera"}},
            {"statik",      {"statik"}},
            {"Statik",      {"statik"}},
            {"STATİK",      {"statik"}},
            {"ırmaktı",     {"ırmak"}},
            {"Irmaktı",     {"ırmak"}},
            {"IRMAKTI",     {"ırmak"}},
        });
    }

    Y_UNIT_TEST(LemmatizeWordIssuesRu) {
        TestLemmatizeWord(LANG_RUS, GOOD_LEMMA_THRESHOLD, {
            {"закрой", {"закрой", "закрывать"}},
            {"мыла", {"мыло", "мыть"}},
            {"тем", {"то", "тот", "тем"}},
            {"бывшие", {"бывший", "быть"}},
        });
        TestLemmatizeWord(LANG_RUS, ANY_LEMMA_THRESHOLD, {
            {"тем", {"то", "тот", "тем", "тема"}},
            {"бывшие", {"бывший", "быть"}},
        });
    }

    Y_UNIT_TEST(Names) {
        TestLemmatizeWord(LANG_RUS, GOOD_LEMMA_THRESHOLD, {
            {"Артём", {"артем"}},
            {"Артёма", {"артем", "артема"}},

            {"Валентин", {"валентин"}},
            {"Валентина", {"валентина", "валентин"}},
            {"Валентину", {"валентина", "валентин"}},
            {"Валентине", {"валентина", "валентин"}},
        });
    }

    Y_UNIT_TEST(Surnames) {
        TestLemmatizeWord(LANG_RUS, GOOD_LEMMA_THRESHOLD, {
            {"Кузнецов", {"кузнецов"}},
            {"Кузнецова", {"кузнецов", "кузнецова"}},
            {"Кузнецову", {"кузнецов", "кузнецова"}},
            {"Кузнецовой", {"кузнецова"}},

            {"Попов", {"попов", "поп"}},
            {"Попова", {"попов", "попова"}},
            {"Попову", {"попов", "попова"}},
            {"Поповой", {"попова", "попов"}},

            {"Тимов", {"тим", "тимов"}},
            {"Тимова", {"тимов", "тимова"}},
            {"Тимову", {"тимов", "тимова"}},
            {"Тимовой", {"тимова"}},

            {"Белых", {"белый", "белых"}},

            {"Собчак", {"собчак"}},
            {"Собчаку", {"собчак"}},

            {"Фихтенгольц", {"фихтенгольц"}},
            {"Фихтенгольцу", {"фихтенголец"}}, // bad
        });
    }

    Y_UNIT_TEST(TestLemmatizeWordBestRu) {
        TestLemmatizeWordBest(LANG_RUS, {
            {"два",                             "два"},
            {"двух",                            "два"},
            {"двойка",                          "двойка"},
            {"двоечка",                         "двоечка"},
            {"пара",                            "пара"},
            {"второй",                          "второй"},
            {"двое",                            "двое"},
            {"двойной",                         "двойной"},
            {"двоичный",                        "двоичный"},
            {"вторых",                          "второй"},
            {"треть",                           "треть"},
            {"Сорока",                          "сорок"},
            {"ЧЕТЫРЁХ",                         "четыре"},
            {"ноль",                            "ноль"},
            {"нуль",                            "ноль"},
            {"нулевой",                         "нулевой"},
            {"полтораста",                      "полтораста"},
            {"третий",                          "третий"},
            {"шеститысячный",                   "шеститысячный"},
            {"двадцатипятитысячному",           "двадцатипятитысячный"},
            {"стосорокашестисотмиллионному",    "стосорокашестисотмиллионный"},
            {"тридцатиодномиллиардный",         "тридцатиодномиллиардный"},
            {"мёд",                             "мед"},
            {"МЁДУ",                            "мед"},
            {"8",                               "8"},
            {"ка",                              "ка"},
            {"Мне",                             "я"},
            {"то",                              "то"},
        });
    }

    Y_UNIT_TEST(TestLemmatizeWordBestTr) {
        TestLemmatizeWordBest(LANG_TUR, {
            {"statik", "statik"},
            {"Statik", "statik"},
            {"STATİK", "statik"},

            {"hız", "hız"},
            {"Hız", "hız"},
            {"HIZ", "hız"},

            {"kamerası", "kamera"},
            {"Kamerası", "kamera"},
            {"KAMERASI", "kamera"},

            {"ırmaktı", "ırmak"},
            {"Irmaktı", "ırmak"},
            {"IRMAKTI", "ırmak"},
        });
    }
}
