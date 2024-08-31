#include "inflector.h"
#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/libs/ut_utils/ut_utils.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(Inflector) {

    // Grammeme names:
    //   kernel/lemmer/dictlib/grammar_enum.h
    //   kernel/lemmer/dictlib/ccl.cpp
    struct TTestData {
        TString Phrase;
        TString DestGrams;
        TString Expected;
    };

    void Test(ELanguage lang, const TVector<TTestData>& datas) {
        for (const TTestData& data : datas) {
            const TGramBitSet grams = TGramBitSet::FromString(data.DestGrams);
            const TString actual = InflectPhrase(data.Phrase, lang, grams);
            UNIT_ASSERT_STRINGS_EQUAL_C(data.Expected, actual, "dest grams: " << grams.ToString(","));
        }
    }

    Y_UNIT_TEST(Was) {
        Test(LANG_RUS, {
            {"был", "",     "был"},
            {"был", "m",    "был"},
            {"был", "f",    "была"},
            {"был", "n",    "было"},
            {"был", "pl",   "были"},
            {"был", "mf",   "есть"}, // error?
        });
    }

    Y_UNIT_TEST(Is) {
        Test(LANG_RUS, {
            {"есть", "",    "есть"},
            {"есть", "m",   "был"}, // error, should be "есть"
            {"есть", "f",   "есть"},
            {"есть", "n",   "есть"},
            {"есть", "pl",  "есть"},
            {"есть", "mf",  "есть"},
        });
    }
}
