#include <alice/bass/forms/market/market_checkout.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NBASS;

namespace {

    bool IsValidEmail(TStringBuf str)
    {
        auto email = TString{str};
        return NMarket::TMarketCheckout::TryFormatEmail(email);
    }

    bool IsValidPhone(TStringBuf str)
    {
        auto phone = TString{str};
        return NMarket::TMarketCheckout::TryFormatPhone(phone);
    }

    TString FormatPhone(TStringBuf str)
    {
        auto phone = TString{str};
        NMarket::TMarketCheckout::TryFormatPhone(phone);
        return phone;
    }

} //anonymous namespace

Y_UNIT_TEST_SUITE(TMarketCheckoutTest)
{
    Y_UNIT_TEST(EmailTest)
    {
        UNIT_ASSERT_EQUAL(IsValidEmail(TStringBuf("qweqwe@yandex.ru")), true);
        UNIT_ASSERT_EQUAL(IsValidEmail(TStringBuf("abcde@google.com")), true);
        UNIT_ASSERT_EQUAL(IsValidEmail(TStringBuf("abcde@mail.com.com")), true);
        UNIT_ASSERT_EQUAL(IsValidEmail(TStringBuf(" spaces@yandex.ru   ")), true);
        UNIT_ASSERT_EQUAL(IsValidEmail(TStringBuf("upperCASE@yandex.ru")), true);
        UNIT_ASSERT_EQUAL(IsValidEmail(TStringBuf("point.point@yandex.ru")), true);

        UNIT_ASSERT_EQUAL(IsValidEmail(TStringBuf("")), false);
        UNIT_ASSERT_EQUAL(IsValidEmail(TStringBuf("dfdffd@yandex")), false);
        UNIT_ASSERT_EQUAL(IsValidEmail(TStringBuf("qweqwe")), false);
        UNIT_ASSERT_EQUAL(IsValidEmail(TStringBuf("@yandex.ru")), false);
        UNIT_ASSERT_EQUAL(IsValidEmail(TStringBuf(".@ya.ru")), false);
        UNIT_ASSERT_EQUAL(IsValidEmail(TStringBuf("yandexyandex.ru")), false);
        UNIT_ASSERT_EQUAL(IsValidEmail(TStringBuf("yandex$yandex.ru")), false);
        UNIT_ASSERT_EQUAL(IsValidEmail(TStringBuf("русскиебуквы@yandex.ru")), false);
    }

    Y_UNIT_TEST(PhoneTest)
    {
        UNIT_ASSERT_EQUAL(IsValidPhone(TStringBuf("")), false);
        UNIT_ASSERT_EQUAL(IsValidPhone(TStringBuf("0")), false);
        UNIT_ASSERT_EQUAL(IsValidPhone(TStringBuf("-1")), false);
        UNIT_ASSERT_EQUAL(IsValidPhone(TStringBuf("+89168751594")), true);
        UNIT_ASSERT_EQUAL(IsValidPhone(TStringBuf("+79168751593")), true);
        UNIT_ASSERT_EQUAL(IsValidPhone(TStringBuf("+7 916 875 15 93")), true);
        UNIT_ASSERT_EQUAL(IsValidPhone(TStringBuf("+7(916)-875-15-93")), true);
        UNIT_ASSERT_EQUAL(IsValidPhone(TStringBuf("+7(916) 875 15 93")), true);
        UNIT_ASSERT_EQUAL(IsValidPhone(TStringBuf("8 (916) 875 15 93")), true);
    }

    Y_UNIT_TEST(FormatPhoneTest)
    {
        UNIT_ASSERT_STRINGS_EQUAL(FormatPhone(TStringBuf("+89168751594")), "+79168751594");
        UNIT_ASSERT_STRINGS_EQUAL(FormatPhone(TStringBuf("+79168751593")), "+79168751593");
        UNIT_ASSERT_STRINGS_EQUAL(FormatPhone(TStringBuf("+7 916 875 15 93")), "+79168751593");
        UNIT_ASSERT_STRINGS_EQUAL(FormatPhone(TStringBuf("+7(916)-875-15-93")), "+79168751593");
        UNIT_ASSERT_STRINGS_EQUAL(FormatPhone(TStringBuf("+7(916) 875 15 93")), "+79168751593");
        UNIT_ASSERT_STRINGS_EQUAL(FormatPhone(TStringBuf("8 (916) 875 15 93")), "+79168751593");
        UNIT_ASSERT_STRINGS_EQUAL(FormatPhone(TStringBuf("8 916 87 515 93.")), "+79168751593");
    }
}
