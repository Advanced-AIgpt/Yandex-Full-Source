#include "ar_normalize.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NNlu;
using namespace NImpl;

Y_UNIT_TEST_SUITE(ArabicNormalize) {

    Y_UNIT_TEST(Dediac) {
        UNIT_ASSERT_EQUAL(Dediac(""), "");
        UNIT_ASSERT_EQUAL(Dediac("هَلْ ذَهَبْتَ إِلَى المَكْتَبَةِ؟"), "هل ذهبت إلى المكتبة؟");
        UNIT_ASSERT_EQUAL(Dediac("لِنَذْهَبْ إِلَى السِّيْنَمَا."), "لنذهب إلى السينما.");
        UNIT_ASSERT_EQUAL(Dediac("اِسْتَوْحَش ل، اِشْتَاق، اِشْتَاق إلى، اِشْتَاق ل، اِشْتَهَى، تَاق، تَاق إلى، تَحَسّر على، تَشَوّف"),
                          "استوحش ل، اشتاق، اشتاق إلى، اشتاق ل، اشتهى، تاق، تاق إلى، تحسر على، تشوف");
    }

    Y_UNIT_TEST(NormalizeUnicode) {
        UNIT_ASSERT_EQUAL(NormalizeUnicode(""), "");
        UNIT_ASSERT_EQUAL(NormalizeUnicode("C\u0327"), "\u00c7");
        UNIT_ASSERT_EQUAL(NormalizeUnicode("ﰷ"), "\u0643\u0627");
        UNIT_ASSERT_EQUAL(NormalizeUnicode("ﷺ"), "\u0635\u0644\u0649 \u0627\u0644\u0644\u0647 \u0639\u0644\u064a\u0647 \u0648\u0633\u0644\u0645");
        UNIT_ASSERT_EQUAL(NormalizeUnicode("\ufdfc"), "\u0631\u064a\u0627\u0644");
        UNIT_ASSERT_EQUAL(NormalizeUnicode("\ufdfd"),
                          "\u0628\u0633\u0645 \u0627\u0644\u0644\u0647 \u0627\u0644\u0631\u062d\u0645\u0646 \u0627\u0644\u0631\u062d\u064a\u0645");
        UNIT_ASSERT_EQUAL(NormalizeUnicode("هَلْ ذَهَبْتَ إِلَى المَكْتَبَةِ ﰷ؟"),
                          "\u0647\u064e\u0644\u0652 \u0630\u064e\u0647\u064e\u0628\u0652\u062a\u064e "
                          "\u0625\u0650\u0644\u064e\u0649 \u0627\u0644\u0645\u064e\u0643\u0652\u062a\u064e\u0628\u064e\u0629\u0650 \u0643\u0627\u061f");
    }

    Y_UNIT_TEST(NormalizeAlef) {
        UNIT_ASSERT_EQUAL(NormalizeAlef(""), "");
        UNIT_ASSERT_EQUAL(NormalizeAlef("المَكْتَبَةِ إأ ٱآ"), "المَكْتَبَةِ اا اا");
    }

    Y_UNIT_TEST(NormalizeTehMarbuta) {
        UNIT_ASSERT_EQUAL(NormalizeTehMarbuta(""), "");
        UNIT_ASSERT_EQUAL(NormalizeTehMarbuta("المَكْتَبَةِ هة"), "المَكْتَبَهِ هه");
    }

    Y_UNIT_TEST(NormalizeArabicText) {
        UNIT_ASSERT_EQUAL(NormalizeArabicText(""), "");
        UNIT_ASSERT_EQUAL(NormalizeArabicText("9:00 أليسا، اضبطي المنبه في أيام الأربعاء على الساعة"),
                          "9:00 اليسا، اضبطي المنبه في ايام الاربعاء على الساعه");
        UNIT_ASSERT_EQUAL(NormalizeArabicText("هَلْ ذَهَبْتَ إِلَى المَكْتَبَةِ ﰷ؟"), "هل ذهبت الى المكتبه كا؟");
    }

}
