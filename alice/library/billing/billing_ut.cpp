#include "billing.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NBilling {

Y_UNIT_TEST_SUITE(BillingTest) {

Y_UNIT_TEST(RequestPromoUrlPath) {
    UNIT_ASSERT_STRINGS_EQUAL(AppHostRequestPromoUrlPath(),
                              "/requestPlus?sendPush=false&sendPersonalCards=false");

    UNIT_ASSERT_STRINGS_EQUAL(AppHostRequestPromoUrlPath(/* sendPush= */ true, /* sendPersonalCards= */ true),
                              "/requestPlus?sendPush=true&sendPersonalCards=true");
}

Y_UNIT_TEST(ParseBillingCorrectResponsePositive) {
    const auto responseJsonStr = TStringBuf(R"({
        "result": "PROMO_AVAILABLE",
        "activate_promo_uri": "http://nonexistentdomain.yandex.net/activate_promo_uri"
    })");

    auto result = std::get<NAlice::NBilling::TPromoAvailability>(ParseBillingResponse(responseJsonStr));
    UNIT_ASSERT(result.IsAvailable);
    UNIT_ASSERT_STRINGS_EQUAL(result.ActivatePromoUri, "http://nonexistentdomain.yandex.net/activate_promo_uri");
    UNIT_ASSERT_STRINGS_EQUAL(result.ExtraPeriodExpiresDate, "");
}

Y_UNIT_TEST(ParseBillingCorrectResponseNegative) {
    const auto responseJsonStr = TStringBuf(R"({
        "result": "PAYMENT_REQUIRED"
    })");

    auto result = std::get<NAlice::NBilling::TPromoAvailability>(ParseBillingResponse(responseJsonStr));
    UNIT_ASSERT(!result.IsAvailable);
    UNIT_ASSERT(result.ActivatePromoUri.empty());
}

Y_UNIT_TEST(ParseBillingCorrectResponsePositiveWithExtraPeriodNotExp) {
    const auto responseJsonStr = TStringBuf(R"({
        "result": "PROMO_AVAILABLE",
        "activate_promo_uri": "http://nonexistentdomain.yandex.net/activate_promo_uri",
        "expiration_time": "2021-09-20"
    })");

    auto result = std::get<NAlice::NBilling::TPromoAvailability>(ParseBillingResponse(responseJsonStr));
    UNIT_ASSERT(result.IsAvailable);
    UNIT_ASSERT_STRINGS_EQUAL(result.ActivatePromoUri, "http://nonexistentdomain.yandex.net/activate_promo_uri");
    UNIT_ASSERT_STRINGS_EQUAL(result.ExtraPeriodExpiresDate, "");
}


Y_UNIT_TEST(ParseBillingCorrectResponsePositiveWithExtraPeriodInExp) {
    const auto responseJsonStr = TStringBuf(R"({
        "result": "PROMO_AVAILABLE",
        "activate_promo_uri": "http://nonexistentdomain.yandex.net/activate_promo_uri",
        "expiration_time": "2021-09-20",
        "experiment": "plus120"
    })");

    auto result = std::get<NAlice::NBilling::TPromoAvailability>(ParseBillingResponse(responseJsonStr));
    UNIT_ASSERT(result.IsAvailable);
    UNIT_ASSERT_STRINGS_EQUAL(result.ActivatePromoUri, "http://nonexistentdomain.yandex.net/activate_promo_uri");
    UNIT_ASSERT_STRINGS_EQUAL(result.ExtraPeriodExpiresDate, "2021-09-20");
}

Y_UNIT_TEST(ParseBillingCorrectResponsePositiveWithoutActivateUri) {
    const auto responseJsonStr = TStringBuf(R"({
        "result": "PROMO_AVAILABLE",
        "expiration_time": "2021-09-20",
        "experiment": "plus120"
    })");
    auto result = std::get<NAlice::NBilling::TPromoAvailability>(ParseBillingResponse(responseJsonStr, false /* needActivatePromoUri */));
    UNIT_ASSERT(result.IsAvailable);
    UNIT_ASSERT_STRINGS_EQUAL(result.ActivatePromoUri, "");
    UNIT_ASSERT_STRINGS_EQUAL(result.ExtraPeriodExpiresDate, "2021-09-20");
}

Y_UNIT_TEST(ParseBillingIncorrectStatus) {
    const auto responseJsonStr = TStringBuf(R"({
        "result": "wrong status"
    })");

    auto error = std::get<NAlice::NBilling::TBillingError>(ParseBillingResponse(responseJsonStr));
    UNIT_ASSERT_STRINGS_EQUAL(error.Message(), "Unexpected billing status: \"wrong status\"");
}

Y_UNIT_TEST(ParseBillingIncorrectActivateUri) {
    const auto responseJsonStr = TStringBuf(R"({
        "result": "PROMO_AVAILABLE"
    })");

    auto error = std::get<NAlice::NBilling::TBillingError>(ParseBillingResponse(responseJsonStr));
    UNIT_ASSERT_STRINGS_EQUAL(error.Message(), "activate_promo_uri is required");
}

Y_UNIT_TEST(ParseBillingIncorrectExpirationDateInExp) {
    const auto responseJsonStr = TStringBuf(R"({
        "result": "PROMO_AVAILABLE",
        "activate_promo_uri": "http://nonexistentdomain.yandex.net/activate_promo_uri",
        "expiration_time": "2021-09",
        "experiment": "plus120"
    })");

    auto error = std::get<NAlice::NBilling::TBillingError>(ParseBillingResponse(responseJsonStr));
    UNIT_ASSERT_STRINGS_EQUAL(error.Message(), "Incorrect extra period expiration date: 2021-09");
}

Y_UNIT_TEST(ParseBillingIncorrectExpirationDateNotExp) {
    const auto responseJsonStr = TStringBuf(R"({
        "result": "PROMO_AVAILABLE",
        "activate_promo_uri": "http://nonexistentdomain.yandex.net/activate_promo_uri",
        "expiration_time": "2021-09"
    })");

    auto result = std::get<NAlice::NBilling::TPromoAvailability>(ParseBillingResponse(responseJsonStr));
    UNIT_ASSERT(result.IsAvailable);
    UNIT_ASSERT_STRINGS_EQUAL(result.ActivatePromoUri, "http://nonexistentdomain.yandex.net/activate_promo_uri");
    UNIT_ASSERT_STRINGS_EQUAL(result.ExtraPeriodExpiresDate, "");
}

Y_UNIT_TEST(FilterBillingExperiments) {
    const THashMap<TString, TMaybe<TString>> expFlags = {
        {"set_billing_exp=flag_1", Nothing()},
        {"prefix=flag_2", Nothing()},
        {"set_billing_exp=flag_3", "flag_value"},
    };
    const TVector<TString> billingExpFlags = NImpl::FilterBillingExperiments(expFlags);

    UNIT_ASSERT(billingExpFlags.size() == 2);
    UNIT_ASSERT(
        (billingExpFlags[0] == "flag_1" and billingExpFlags[1] == "flag_3") or
        (billingExpFlags[0] == "flag_3" and billingExpFlags[1] == "flag_1")
    );
}

Y_UNIT_TEST(FilterBillingExperimentsEmpty) {
    const THashMap<TString, TMaybe<TString>> expFlags = {
        {"prefix_1=flag_1", Nothing()},
        {"prefix_2=flag_2", Nothing()}
    };
    const TVector<TString> billingExpFlags = NImpl::FilterBillingExperiments(expFlags);
    UNIT_ASSERT(billingExpFlags.empty());
}


Y_UNIT_TEST(ExpFlagsToJson) {
    const TVector<TString> expFlags = {"flag_1", "flag_2"};
    auto expFlagsJson = NImpl::ExpFlagsToJson(expFlags);
    UNIT_ASSERT_STRINGS_EQUAL(
        expFlagsJson,
        "[{\"CONTEXT\":{\"MAIN\":{\"VOICE\":{\"flags\":[\"flag_1\",\"flag_2\"]}}},\"HANDLER\":\"VOICE\"}]"
    );
}

Y_UNIT_TEST(ExpFlagsToBillingHeader) {
    const THashMap<TString, TMaybe<TString>> expFlags = {
        {"set_billing_exp=flag_1", Nothing()},
        {"prefix=flag_2", Nothing()},
    };
    auto expFlagsJson = ExpFlagsToBillingHeader(expFlags);

    UNIT_ASSERT(expFlagsJson);
    UNIT_ASSERT_STRINGS_EQUAL(
        *expFlagsJson,
        "[{\"CONTEXT\":{\"MAIN\":{\"VOICE\":{\"flags\":[\"flag_1\"]}}},\"HANDLER\":\"VOICE\"}]"
    );
}

Y_UNIT_TEST(ExpFlagsToBillingHeaderEmpty) {
    const THashMap<TString, TMaybe<TString>> expFlags = {
        {"prefix_1=flag_1", Nothing()},
        {"prefix_2=flag_2", Nothing()},
    };
    auto expFlagsJson = ExpFlagsToBillingHeader(expFlags);
    UNIT_ASSERT(expFlagsJson.Empty());
}

}

} // namespace NAlice::NBilling
