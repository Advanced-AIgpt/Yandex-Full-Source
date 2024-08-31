#include <alice/matrix/notificator/library/utils/utils.h>

#include <library/cpp/testing/gtest/gtest.h>

TEST(TMatrixNotificatorUtilsTest, TestTryParseFromString) {
    {
        auto result = NMatrix::NNotificator::TryParseFromString("123", "var_name");
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.Success(), 123ull);
    }

    {
        auto result = NMatrix::NNotificator::TryParseFromString("kek", "var_name");
        ASSERT_TRUE(result.IsError());
        EXPECT_THAT(result.Error(), ::testing::HasSubstr("Unable to cast var_name 'kek' to ui64"));
    }
}
