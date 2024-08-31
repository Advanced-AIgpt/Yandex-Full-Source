#include <alice/matrix/notificator/library/user_white_list/user_white_list.h>

#include <library/cpp/testing/gtest/gtest.h>

namespace {

NMatrix::NNotificator::TUserWhiteListSettings GetConfig(bool enabled) {
    NMatrix::NNotificator::TUserWhiteListSettings config;
    config.SetEnabled(enabled);
    *config.AddPuids() = "puid1";
    *config.AddPuids() = "puid2";

    return config;
}

} // namespace

TEST(TMatrixNotificatorDeliveryServiceUserWhiteListTest, TestEnabled) {
    NMatrix::NNotificator::TUserWhiteList userWhiteList(GetConfig(true));

    EXPECT_FALSE(userWhiteList.IsPuidAllowedToProcess("puid0"));
    EXPECT_TRUE(userWhiteList.IsPuidAllowedToProcess("puid1"));
    EXPECT_TRUE(userWhiteList.IsPuidAllowedToProcess("puid2"));
    EXPECT_FALSE(userWhiteList.IsPuidAllowedToProcess("puid3"));
}

TEST(TMatrixNotificatorDeliveryServiceUserWhiteListTest, TestDisabled) {
    NMatrix::NNotificator::TUserWhiteList userWhiteList(GetConfig(false));

    EXPECT_TRUE(userWhiteList.IsPuidAllowedToProcess("puid0"));
    EXPECT_TRUE(userWhiteList.IsPuidAllowedToProcess("puid1"));
    EXPECT_TRUE(userWhiteList.IsPuidAllowedToProcess("puid2"));
    EXPECT_TRUE(userWhiteList.IsPuidAllowedToProcess("puid3"));
}
