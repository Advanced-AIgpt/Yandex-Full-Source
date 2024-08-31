#include <alice/bass/forms/market/util/amount.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TMarketAmountParsingTest)
{
    // MALISA-283
    Y_UNIT_TEST(NormalizeAmountTest)
    {
        using NBASS::NMarket::NormalizeAmount;

        UNIT_ASSERT_EQUAL(NormalizeAmount("500"), 500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("1000"), 1000);
        UNIT_ASSERT_EQUAL(NormalizeAmount("2000"), 2000);
        UNIT_ASSERT_EQUAL(NormalizeAmount("5000"), 5000);

        UNIT_ASSERT_EQUAL(NormalizeAmount("2 500"), 2500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("5 200"), 5200);
        UNIT_ASSERT_EQUAL(NormalizeAmount("7 500"), 7500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("7 200"), 7200);

        UNIT_ASSERT_EQUAL(NormalizeAmount("1,5 тысячи"), 1500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("1,5 тысяч"), 1500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("1,5 1000"), 1500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("2,5 тысячи"), 2500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("2,5 тысяч"), 2500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("2,5 1000"), 2500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("3,5 тысячи"), 3500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("3,5 тысяч"), 3500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("3,5 1000"), 3500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("10,5 тысяч"), 10500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("10,5 1000"), 10500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("17,5 тысяч"), 17500);
        UNIT_ASSERT_EQUAL(NormalizeAmount("17,5 1000"), 17500);

        UNIT_ASSERT_EQUAL(NormalizeAmount("100 500"), 0);
        UNIT_ASSERT_EQUAL(NormalizeAmount("200 300"), 0);
        UNIT_ASSERT_EQUAL(NormalizeAmount("3 50"), 0);

        UNIT_ASSERT_EQUAL(NormalizeAmount("какой-то бред"), 0);
    }
}
