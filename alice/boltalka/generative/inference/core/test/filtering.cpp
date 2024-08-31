#include <alice/boltalka/generative/inference/core/model.h>

#include <dict/dictutil/str.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NGenerativeBoltalka;


Y_UNIT_TEST_SUITE(TestFilters) {
        IGenerativeFilter::TParams CreateEmptyFilteringParams() {
            IGenerativeFilter::TParams params;
            params.FilterEmpty = false;
            params.FilterDuplicateWords = false;
            params.FilterBadWords = false;
            params.FilterDuplicateNGrams = false;
            params.FilterByUniqueWordsRatio = false;
            return params;
        }

        TGenerativeResponse CreateResponse(TString str) {
            return TGenerativeResponse(str, 0.0, 1);
        }

        Y_UNIT_TEST(TestFilterEmpty) {
            IGenerativeFilter::TParams params = CreateEmptyFilteringParams();
            params.FilterEmpty = true;

            auto filter = CreateFilter(params);

            UNIT_ASSERT_EQUAL(true, filter->ShouldFilterResponse(CreateResponse("")));
            UNIT_ASSERT_EQUAL(false, filter->ShouldFilterResponse(CreateResponse("не пусто")));
        }

        Y_UNIT_TEST(TestFilterDuplicateWords) {
            IGenerativeFilter::TParams params = CreateEmptyFilteringParams();
            params.FilterDuplicateWords = true;

            auto filter = CreateFilter(params);

            UNIT_ASSERT_EQUAL(false, filter->ShouldFilterResponse(CreateResponse("а"))); // only one
            UNIT_ASSERT_EQUAL(false, filter->ShouldFilterResponse(CreateResponse("а б в г д"))); // all unique
            UNIT_ASSERT_EQUAL(true, filter->ShouldFilterResponse(CreateResponse("а а в г д"))); // start and next
            UNIT_ASSERT_EQUAL(true, filter->ShouldFilterResponse(CreateResponse("а б а г д"))); // start and after next
            UNIT_ASSERT_EQUAL(true, filter->ShouldFilterResponse(CreateResponse("а б в г а"))); // start and end
        }

        Y_UNIT_TEST(TestFilterDuplicateNGrams) {
            IGenerativeFilter::TParams params = CreateEmptyFilteringParams();
            params.FilterDuplicateNGrams = true;
            params.NGramSize = 3;

            auto filter = CreateFilter(params);

            UNIT_ASSERT_EQUAL(false, filter->ShouldFilterResponse(CreateResponse("а б в"))); // 1 3-gram
            UNIT_ASSERT_EQUAL(false, filter->ShouldFilterResponse(CreateResponse("а б в а б г а б д"))); // 2-grams only
            UNIT_ASSERT_EQUAL(false, filter->ShouldFilterResponse(CreateResponse("а б в д е ж з и к л м"))); // all unique
            UNIT_ASSERT_EQUAL(true, filter->ShouldFilterResponse(CreateResponse("а б в д е ж з и а б в"))); // start and end
            UNIT_ASSERT_EQUAL(true, filter->ShouldFilterResponse(CreateResponse("а б в д е а б в ж з и"))); // start and middle
            UNIT_ASSERT_EQUAL(true, filter->ShouldFilterResponse(CreateResponse("а б в а б в д е ж з и"))); // consecutive
            UNIT_ASSERT_EQUAL(true, filter->ShouldFilterResponse(CreateResponse("а б а б а д е ж з и к"))); // overlapping
        }

        Y_UNIT_TEST(TestFilterByUniqueWordsRatio) {
            IGenerativeFilter::TParams params = CreateEmptyFilteringParams();
            params.FilterByUniqueWordsRatio = true;
            params.MinUniqueRatio = 0.67;

            auto filter = CreateFilter(params);

            UNIT_ASSERT_EQUAL(false, filter->ShouldFilterResponse(CreateResponse("а"))); // 1.0
            UNIT_ASSERT_EQUAL(false, filter->ShouldFilterResponse(CreateResponse("а б"))); // 1.0
            UNIT_ASSERT_EQUAL(false, filter->ShouldFilterResponse(CreateResponse("а б в"))); // 1.0
            UNIT_ASSERT_EQUAL(true, filter->ShouldFilterResponse(CreateResponse("а а"))); // 0.5
            UNIT_ASSERT_EQUAL(false, filter->ShouldFilterResponse(CreateResponse("а б"))); // 1.0
            UNIT_ASSERT_EQUAL(true, filter->ShouldFilterResponse(CreateResponse("а а б"))); // 0.667
            UNIT_ASSERT_EQUAL(false, filter->ShouldFilterResponse(CreateResponse("а б в"))); // 1.0
            UNIT_ASSERT_EQUAL(true, filter->ShouldFilterResponse(CreateResponse("а а б б"))); // 0.5
            UNIT_ASSERT_EQUAL(false, filter->ShouldFilterResponse(CreateResponse("а а б в"))); // 0.75
        }
}
