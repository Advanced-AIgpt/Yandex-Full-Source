#include "common.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NNetwork {

TString GetBlurValue(const TString& uuid, const ui64 seed, const ui32 blurRatio);

}

namespace {

using namespace NAlice::NNetwork;

Y_UNIT_TEST_SUITE(AliceNetwork) {
    Y_UNIT_TEST(AppendUriNormalizationPath) {
        UNIT_ASSERT_EQUAL(AppendToUri(ParseUri("http://ya.ru/"), "/search").PrintS(), "http://ya.ru/search");

        auto uri = ParseUri("http://ya.ru/");
        AppendToUriInPlace(uri, "/search");
        UNIT_ASSERT_EQUAL(uri.PrintS(), "http://ya.ru/search");
    }

    Y_UNIT_TEST(GetBlurValue) {
        const auto getBlurValue =
            std::bind(GetBlurValue, /* uuid= */ "uuid", /* seed= */ 32, /* blurRatio= */ std::placeholders::_1);
        UNIT_ASSERT_VALUES_EQUAL(getBlurValue(/* blurRatio= */ 0), Default<TString>());
        UNIT_ASSERT_VALUES_EQUAL(getBlurValue(/* blurRatio= */ 1), Default<TString>());
        UNIT_ASSERT_VALUES_EQUAL(getBlurValue(/* blurRatio= */ 2), "0");
        UNIT_ASSERT_VALUES_EQUAL(getBlurValue(/* blurRatio= */ 10), "6");
        // check that there are no random for same values
        UNIT_ASSERT_VALUES_EQUAL(getBlurValue(/* blurRatio= */ 2), "0");
        UNIT_ASSERT_VALUES_EQUAL(getBlurValue(/* blurRatio= */ 10), "6");
    }

    Y_UNIT_TEST(GetVinsBalancerHintUtterance) {
        const TString defaultUtterance = "utterance";
        const TString defaultUuid = "yddqd";
        const ui64 defaultSeed = 42;
        const ui32 defaultBlurRatio = 5;

        UNIT_ASSERT_VALUES_EQUAL(12122812675617433969ull, GetVinsBalancerHint(defaultUtterance, defaultUuid, defaultSeed, defaultBlurRatio));
        UNIT_ASSERT_VALUES_EQUAL(16283314894726039323ull, GetVinsBalancerHint("otherutterance", defaultUuid, defaultSeed, defaultBlurRatio));
        UNIT_ASSERT_VALUES_EQUAL(16884584249293684250ull, GetVinsBalancerHint("anotherutterance", defaultUuid, defaultSeed, defaultBlurRatio));

    }
    Y_UNIT_TEST(GetVinsBalancerHintUuid) {
        const TString defaultUtterance = "utterance";
        const TString defaultUuid = "yddqd";
        const ui64 defaultSeed = 42;
        const ui32 defaultBlurRatio = 5;

        UNIT_ASSERT_VALUES_EQUAL(12122812675617433969ull, GetVinsBalancerHint(defaultUtterance, defaultUuid, defaultSeed, defaultBlurRatio));
        UNIT_ASSERT_VALUES_EQUAL(10593449849436172029ull, GetVinsBalancerHint(defaultUtterance, "otheruuid", defaultSeed, defaultBlurRatio));
        UNIT_ASSERT_VALUES_EQUAL(11839267255839172721ull, GetVinsBalancerHint(defaultUtterance, "anotheruuid", defaultSeed, defaultBlurRatio));

    }
    Y_UNIT_TEST(GetVinsBalancerHintSeed) {
        const TString defaultUtterance = "utterance";
        const TString defaultUuid = "yddqd";
        const ui64 defaultSeed = 42;
        const ui32 defaultBlurRatio = 5;

        UNIT_ASSERT_VALUES_EQUAL(12122812675617433969ull, GetVinsBalancerHint(defaultUtterance, defaultUuid, defaultSeed, defaultBlurRatio));
        UNIT_ASSERT_VALUES_EQUAL(12122812675617433969ull, GetVinsBalancerHint(defaultUtterance, defaultUuid, 4242424242, defaultBlurRatio));
        UNIT_ASSERT_VALUES_EQUAL( 3510117823949625396ull, GetVinsBalancerHint(defaultUtterance, defaultUuid, 42424242424, defaultBlurRatio));

    }
    Y_UNIT_TEST(GetVinsBalancerHintBlurRation) {
        const TString defaultUtterance = "utterance";
        const TString defaultUuid = "yddqd";
        const ui64 defaultSeed = 42;
        const ui32 defaultBlurRatio = 5;

        UNIT_ASSERT_VALUES_EQUAL(12122812675617433969ull, GetVinsBalancerHint(defaultUtterance, defaultUuid, defaultSeed, defaultBlurRatio));
        UNIT_ASSERT_VALUES_EQUAL( 6503128262529047049ull, GetVinsBalancerHint(defaultUtterance, defaultUuid, defaultSeed, 0));
        UNIT_ASSERT_VALUES_EQUAL( 6503128262529047049ull, GetVinsBalancerHint(defaultUtterance, defaultUuid, defaultSeed, 1));
        UNIT_ASSERT_VALUES_EQUAL(12122812675617433969ull, GetVinsBalancerHint(defaultUtterance, defaultUuid, defaultSeed, 2));
        UNIT_ASSERT_VALUES_EQUAL( 3510117823949625396ull, GetVinsBalancerHint(defaultUtterance, defaultUuid, defaultSeed, 4));
    }
}

} // namespace
