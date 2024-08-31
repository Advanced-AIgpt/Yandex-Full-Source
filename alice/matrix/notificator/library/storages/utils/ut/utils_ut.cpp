#include <alice/matrix/notificator/library/storages/utils/utils.h>

#include <library/cpp/testing/gtest/gtest.h>

TEST(TMatrixNotificatorStoragesUtilsTest, TestLegacyComputeShardKey) {
    // MD5("test") == 098f6bcd4621d373|cade4e832627b4f6
    EXPECT_EQ(NMatrix::NNotificator::LegacyComputeShardKey("test"), 0xCADE4E832627B4F6ull);

    // MD5("test_other") == a8360b0b0b2eed8d|185738536ff5b841
    EXPECT_EQ(NMatrix::NNotificator::LegacyComputeShardKey("test_other"), 0x185738536FF5B841ull);
}

TEST(TMatrixNotificatorStoragesUtilsTest, TestTryParseSupportedFeatureFromString) {
    EXPECT_EQ(NMatrix::NNotificator::TryParseSupportedFeatureFromString("random"), Nothing());

    EXPECT_EQ(NMatrix::NNotificator::TryParseSupportedFeatureFromString("unknown"), NMatrix::NApi::TUserDeviceInfo::UNKNOWN);
    EXPECT_EQ(NMatrix::NNotificator::TryParseSupportedFeatureFromString("audio_client"), NMatrix::NApi::TUserDeviceInfo::AUDIO_CLIENT);
}

TEST(TMatrixNotificatorStoragesUtilsTest, SupportedFeatureToString) {
    EXPECT_EQ(NMatrix::NNotificator::SupportedFeatureToString(NMatrix::NApi::TUserDeviceInfo::UNKNOWN), "unknown");
    EXPECT_EQ(NMatrix::NNotificator::SupportedFeatureToString(NMatrix::NApi::TUserDeviceInfo::AUDIO_CLIENT), "audio_client");
    EXPECT_EQ(
        NMatrix::NNotificator::SupportedFeatureToString(
            NMatrix::NApi::TUserDeviceInfo::ESupportedFeature::TUserDeviceInfo_ESupportedFeature_TUserDeviceInfo_ESupportedFeature_INT_MIN_SENTINEL_DO_NOT_USE_
        ),
        "unknown"
    );
}
