#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/vector.h>
#include <util/string/split.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(StringUtils) {

    Y_UNIT_TEST(StripStrings) {
        const TString string = "12, 34\t, \t,";
        const TVector<TString> strings = {"12", "  34\t", " \t", ""};
        const TVector<TString> expectedKeepEmpty = {"12", "34", "", ""};
        const TVector<TString> expectedDropEmpty = {"12", "34"};
        UNIT_ASSERT_EQUAL(StripStrings(strings, false), expectedKeepEmpty);
        UNIT_ASSERT_EQUAL(StripStrings(strings, true), expectedDropEmpty);
        UNIT_ASSERT_EQUAL(StripStrings(StringSplitter(string).Split(',').ToList<TString>()), expectedKeepEmpty);
        UNIT_ASSERT_EQUAL(StripStrings(StringSplitter(string).Split(',').SkipEmpty().ToList<TString>(), true), expectedDropEmpty);
    }
}
