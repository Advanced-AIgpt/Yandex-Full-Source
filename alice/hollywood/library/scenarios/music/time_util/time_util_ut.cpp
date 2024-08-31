#include "time_util.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

Y_UNIT_TEST_SUITE(TInstantFormat) {

Y_UNIT_TEST(TInstantFormat) {
    auto timestamp = TInstant::FromValue(1615912017123456);
    UNIT_ASSERT_STRINGS_EQUAL(FormatTInstant(timestamp), "2021-03-16T16:26:57.123Z");
}

Y_UNIT_TEST(TInstantFormatZeroMillis) {
    auto timestamp = TInstant::FromValue(1615912017000000);
    UNIT_ASSERT_STRINGS_EQUAL(FormatTInstant(timestamp), "2021-03-16T16:26:57.000Z");
}

};

}  // namespace NAlice::NHollywood::NMusic
