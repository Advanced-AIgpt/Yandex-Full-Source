#include <alice/library/unittest/ut_helpers.h>
#include <alice/library/response_similarity/response_similarity.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NResponseSimilarity;

namespace {

Y_UNIT_TEST_SUITE(ResponseSimilarity) {
    Y_UNIT_TEST(Main) {
        TSimilarity result1 = CalculateResponseItemSimilarity("другие",
            TStringBuf("лучший фильм про других эвер"), ELanguage::LANG_RUS);
        UNIT_ASSERT_VALUES_EQUAL(result1.GetPrefix().GetMax(), 0.0);
        UNIT_ASSERT_VALUES_EQUAL(result1.GetDoublePrefix().GetMax(), 0.0);
        UNIT_ASSERT_VALUES_EQUAL(result1.GetQueryInResponse().GetMax(), 1.0);

        TSimilarity result2 = CalculateResponseItemSimilarity("человек паук",
            TStringBuf("человек паук возвращение домой"), ELanguage::LANG_RUS);
        UNIT_ASSERT_VALUES_EQUAL(result2.GetPrefix().GetMax(), 1.0);
        UNIT_ASSERT_VALUES_EQUAL(result2.GetDoublePrefix().GetMax(), 0.5);
        UNIT_ASSERT_VALUES_EQUAL(result2.GetQueryInResponse().GetMax(), 1.0);

        TSimilarity result3 = CalculateResponseItemSimilarity("человек паук",
            TStringBuf("возвращение домой человек паук"), ELanguage::LANG_RUS);
        UNIT_ASSERT_VALUES_EQUAL(result3.GetPrefix().GetMax(), 0.0);
        UNIT_ASSERT_VALUES_EQUAL(result3.GetDoublePrefix().GetMax(), 0.5);
        UNIT_ASSERT_VALUES_EQUAL(result3.GetQueryInResponse().GetMax(), 1.0);

        TSimilarity result4 = CalculateResponseItemSimilarity("короткий", TStringBuf("короткий"), ELanguage::LANG_RUS);
        UNIT_ASSERT_VALUES_EQUAL(result4.GetPrefix().GetMax(), 1.0);
        UNIT_ASSERT_VALUES_EQUAL(result4.GetDoublePrefix().GetMax(), 1.0);
        UNIT_ASSERT_VALUES_EQUAL(result4.GetResponseInQuery().GetMax(), 1.0);
        UNIT_ASSERT_VALUES_EQUAL(result4.GetQueryInResponse().GetMax(), 1.0);

        TSimilarity result5 = CalculateResponseItemSimilarity("какой-то очень длинный запрос непонятный",
            TStringBuf("непонятный"), ELanguage::LANG_RUS);
        UNIT_ASSERT_VALUES_EQUAL(result5.GetResponseInQuery().GetMax(), 1.0);

        TSimilarity totalResult = AggregateSimilarity({result1, result2, result3, result4, result5});

        UNIT_ASSERT_VALUES_EQUAL(totalResult.GetPrefix().GetMax(), 1.0);
        UNIT_ASSERT_VALUES_EQUAL(totalResult.GetDoublePrefix().GetMax(), 1.0);
        UNIT_ASSERT_VALUES_EQUAL(totalResult.GetResponseInQuery().GetMax(), 1.0);
        UNIT_ASSERT_VALUES_EQUAL(totalResult.GetQueryInResponse().GetMax(), 1.0);

        UNIT_ASSERT_VALUES_EQUAL(totalResult.GetPrefix().GetMin(), 0.0);
        UNIT_ASSERT_VALUES_EQUAL(totalResult.GetDoublePrefix().GetMin(), 0.0);
        UNIT_ASSERT(totalResult.GetResponseInQuery().GetMin() > 0.1);
        UNIT_ASSERT(totalResult.GetQueryInResponse().GetMin() > 0.1);

        UNIT_ASSERT(totalResult.GetPrefix().GetMean() > 0.4);
        UNIT_ASSERT(totalResult.GetDoublePrefix().GetMean() < 0.7);

        TSimilarity result6 = CalculateResponseItemSimilarity("english",
            TStringBuf("english, do you speak it"), ELanguage::LANG_RUS);
        UNIT_ASSERT_VALUES_EQUAL(result6.GetPrefix().GetMax(), 1.0);

        TSimilarity result7 = CalculateResponseItemSimilarity("человек",
            TStringBuf("\u0007[человек\u0007]"sv), ELanguage::LANG_RUS);
        UNIT_ASSERT_VALUES_EQUAL(result7.GetPrefix().GetMax(), 1.0);
    }
}

} // namespace
