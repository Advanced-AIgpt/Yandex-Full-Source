#include "normalize.h"

#include <alice/nlu/libs/ut_utils/ut_utils.h>
#include <library/cpp/testing/unittest/registar.h>

#include <utility>

using namespace NNlu;

Y_UNIT_TEST_SUITE(Normalize) {

    void TestNormalizeWord(ELanguage lang, const TVector<std::pair<TStringBuf, TStringBuf>>& data) {
        for (const auto& [word, expected] : data) {
            NAlice::NUtUtils::TestEqual(word, expected, NormalizeWord(word, lang));
        }
    }

    void TestNormalizeText(ELanguage lang, const TVector<std::pair<TStringBuf, TStringBuf>>& data) {
        for (const auto& [text, expected] : data) {
            NAlice::NUtUtils::TestEqual(text, expected, NormalizeText(text, lang));
        }
    }

    Y_UNIT_TEST(NormalizeWordRu) {
        TestNormalizeWord(LANG_RUS, {
            {"Моё",     "мое"},
            {"всё",     "все"},
            {"123",     "123"},
            {"АИ92",    "аи92"},
        });
    }

    Y_UNIT_TEST(NormalizeWordTr) {
        TestNormalizeWord(LANG_TUR, {
            {"kamerası",    "kamerası"},
            {"Kamerası",    "kamerası"},
            {"KAMERASI",    "kamerası"},
            {"statik",      "statik"},
            {"Statik",      "statik"},
            {"STATİK",      "statik"},
            {"ırmaktı",     "ırmaktı"},
            {"Irmaktı",     "ırmaktı"},
            {"IRMAKTI",     "ırmaktı"},
        });
    }

    Y_UNIT_TEST(NormalizeTextAr) {
        TestNormalizeText(LANG_ARA, {
            {"",    ""},
            {"9:00 أليسا، اضبطي المنبه في أيام الأربعاء على الساعة",
             "9:00 اليسا، اضبطي المنبه في ايام الاربعاء على الساعه"},
            {"هَلْ ذَهَبْتَ إِلَى المَكْتَبَةِ ﰷ؟",
             "هل ذهبت الى المكتبه كا؟"},
        });
    }

    Y_UNIT_TEST(NormalizeTextMultilingual) {
        TestNormalizeText(LANG_ARA, {
            {"Корова Cow بقرة",     "корова cow بقره"}
        });
        TestNormalizeText(LANG_RUS, {
            {"Утка DUCK клён",   "утка duck клен"}
        });
    }

    Y_UNIT_TEST(NormalizeTextRu) {
        TestNormalizeText(LANG_RUS, {
            {"включи на 10 %", "включи на 10 процент"},
        });
    }
}
