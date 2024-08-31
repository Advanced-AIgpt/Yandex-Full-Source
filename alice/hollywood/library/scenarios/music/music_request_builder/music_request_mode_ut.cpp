#include "music_request_mode.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

Y_UNIT_TEST_SUITE(MusicRequestModeSuite) {

Y_UNIT_TEST(DefaultParamsTest) {
    auto musicRequestModeInfo = TMusicRequestModeInfoBuilder().BuildAndMove();
    UNIT_ASSERT_EQUAL(musicRequestModeInfo.AuthMethod, EAuthMethod::Unknown);
    UNIT_ASSERT_EQUAL(musicRequestModeInfo.RequestMode, ERequestMode::Unknown);
    UNIT_ASSERT_STRINGS_EQUAL(musicRequestModeInfo.OwnerUserId, "unspecified");
    UNIT_ASSERT_STRINGS_EQUAL(musicRequestModeInfo.RequesterUserId, "unspecified");
}

Y_UNIT_TEST(Test) {
    auto musicRequestModeInfo = TMusicRequestModeInfoBuilder()
            .SetAuthMethod(EAuthMethod::UserId)
            .SetRequestMode(ERequestMode::Guest)
            .SetOwnerUserId("1234567890")
            .SetRequesterUserId("9876543210")
            .BuildAndMove();

    UNIT_ASSERT_EQUAL(musicRequestModeInfo.AuthMethod, EAuthMethod::UserId);
    UNIT_ASSERT_EQUAL(musicRequestModeInfo.RequestMode, ERequestMode::Guest);
    UNIT_ASSERT_STRINGS_EQUAL(musicRequestModeInfo.OwnerUserId, "1234567890");
    UNIT_ASSERT_STRINGS_EQUAL(musicRequestModeInfo.RequesterUserId, "9876543210");
}

}

} // namespace NAlice::NHollywood::NMusic
