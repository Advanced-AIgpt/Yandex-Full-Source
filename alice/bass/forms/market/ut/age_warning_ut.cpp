#include <alice/bass/forms/market/types/warning.cpp>

#include <library/cpp/testing/unittest/registar.h>

using namespace NBASS;

Y_UNIT_TEST_SUITE(TMarketAgeWarningTest)
{
    Y_UNIT_TEST(TypeParseTest)
    {
        using NBASS::NMarket::TWarning;
        UNIT_ASSERT_EQUAL(TWarning::GetAge("ijfeafije"), "");
        UNIT_ASSERT_EQUAL(TWarning::GetAge("age_0"), "0");
        UNIT_ASSERT_EQUAL(TWarning::GetAge("age_6"), "6");
        UNIT_ASSERT_EQUAL(TWarning::GetAge("age_12"), "12");
        UNIT_ASSERT_EQUAL(TWarning::GetAge("age_18"), "18");
        UNIT_ASSERT_EQUAL(TWarning::GetAge("age"), "18");
        UNIT_ASSERT_EQUAL(TWarning::GetAge("adult"), "18");
        UNIT_ASSERT_EQUAL(TWarning::GetAge("age_something_strange"), "18");
        UNIT_ASSERT_EQUAL(TWarning::GetAge("age_-7"), "18");
        UNIT_ASSERT_EQUAL(TWarning::GetAge("age_1337"), "18");

        UNIT_ASSERT_EQUAL(TWarning::GetAge("medicine"), "");
        UNIT_ASSERT_EQUAL(TWarning::GetAge("drugs"), "");
        UNIT_ASSERT_EQUAL(TWarning::GetAge("tobacco"), "");
    }
}
