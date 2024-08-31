#include "matcher.h"
#include "string_matcher.h"
#include <library/cpp/testing/unittest/registar.h>
#include <util/charset/wide.h>

namespace NAlice::NHollywood::NFood {

Y_UNIT_TEST_SUITE(StringMatcher) {

    void TestNormalization(TStringBuf original, TStringBuf expected) {
        TStringMatcher matcher;
        matcher.ReadOptionsJson(ReadHardcodedMenuMatcherConfig()["matching_options"]);
        const TString actual = WideToUTF8(matcher.NormalizeForMatch(original));
        UNIT_ASSERT_STRINGS_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(Normalization) {
        TestNormalization("Большие Креветки (6 шт.)", "6 креветки шт");
        TestNormalization("6 штук креветок", "6 креветки шт");
        TestNormalization("Снэк Бокс с крыльями", "бокс крыльями снэк");
    }
}

}  // namespace NAlice::NHollywood::NFood
