#include <alice/matrix/library/version/version.h>

#include <build/scripts/c_templates/svnversion.h>

#include <library/cpp/testing/gtest/gtest.h>

#include <util/string/cast.h>


TEST(TMatrixVersionTest, TestGetVersion) {
    const auto& version = NMatrix::GetVersion();

    static auto getShortStr = [](const TString& st) {
        if (st.size() >= 10) {
            return st.substr(0, 10);
        }
        return st;
    };

    EXPECT_THAT(version, ::testing::HasSubstr(TString::Join('@', ToString(GetArcadiaPatchNumber()))));
    EXPECT_THAT(version, ::testing::HasSubstr(TString::Join('~', getShortStr(GetProgramHash()))));
}
