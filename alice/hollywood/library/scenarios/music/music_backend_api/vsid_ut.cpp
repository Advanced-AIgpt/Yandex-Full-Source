#include "vsid.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

namespace {

Y_UNIT_TEST_SUITE(VsidTest) {

Y_UNIT_TEST(BasicTest) {
    TRng rng(42);
    auto ts = TInstant::Seconds(1623303118);
    auto vsid = MakeHollywoodVsid(rng, ts);
    UNIT_ASSERT_VALUES_EQUAL(vsid.size(), 64);
                                   //         1         2         3         4                       1
                                   //12345678901234567890123456789012345678901234x123x1234x1234567890
    UNIT_ASSERT_STRINGS_EQUAL(vsid, "oyIAlWIOINfgyCp2IWfTPKv1bAlw02dLoaUXIEm99ehFxSTMx0000x1623303118");
}

}

} // namespace

} //namespace NAlice::NHollywood::NMusic
