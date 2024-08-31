#include "factors.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NSearch;

namespace {

Y_UNIT_TEST_SUITE(Search) {
    Y_UNIT_TEST(relev) {
        auto rval = ParseFactors("xy=yac");
        UNIT_ASSERT_VALUES_EQUAL(rval.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(rval["xy"], "yac");

        rval = ParseFactors("xy=");
        UNIT_ASSERT_VALUES_EQUAL(rval.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(rval["xy"], "");

        rval = ParseFactors("xy");
        UNIT_ASSERT_VALUES_EQUAL(rval.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(rval["xy"], "");

        rval = ParseFactors("xy=yac;na=Yac");
        UNIT_ASSERT_VALUES_EQUAL(rval.size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(rval["xy"], "yac");
        UNIT_ASSERT_VALUES_EQUAL(rval["na"], "Yac");

        rval = ParseFactors("xy=yac;na");
        UNIT_ASSERT_VALUES_EQUAL(rval.size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(rval["xy"], "yac");
        UNIT_ASSERT_VALUES_EQUAL(rval["na"], "");

        rval = ParseFactors("xy=yac;na;XY=YAC");
        UNIT_ASSERT_VALUES_EQUAL(rval.size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(rval["xy"], "yac");
        UNIT_ASSERT_VALUES_EQUAL(rval["XY"], "YAC");
        UNIT_ASSERT_VALUES_EQUAL(rval["na"], "");

        rval = ParseFactors(";xy=yac;;na=;XY=YAC");
        UNIT_ASSERT_VALUES_EQUAL(rval.size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(rval["xy"], "yac");
        UNIT_ASSERT_VALUES_EQUAL(rval["XY"], "YAC");
        UNIT_ASSERT_VALUES_EQUAL(rval["na"], "");

        rval = ParseFactors(";;");
        UNIT_ASSERT(rval.empty());
    }
}

} // namespace
