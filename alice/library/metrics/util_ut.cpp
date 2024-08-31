#include "util.h"

#include <library/cpp/testing/unittest/registar.h>

namespace {

Y_UNIT_TEST_SUITE(MetricsUtil) {
    Y_UNIT_TEST(NormalizeSensorNameForGolovan) {
        using namespace NAlice::NMetrics;

        // Chop double '_' prefix and suffix.
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("__test"), "test");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("test__"), "test");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("__test__"), "test");

        // Chop single '_' prefix and suffix.
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("_test_"), "test");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("test_"), "test");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("_test"), "test");

        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("_te__st"), "te_st");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("_te_________st"), "te_st");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("_te____st"), "te_st");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("_te___st"), "te_st");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("_te_st"), "te_st");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("_t__e_s__t"), "t_e_s_t");

        // Copypasted test from bass.
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan(""), "");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("abc"), "abc");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("a_b_c"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("a__b_c"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("a_b__c"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("a_b__c"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("a______b______c"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("aaa_bbb_ccc"), "aaa_bbb_ccc");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("aaaaaa_bbbbbb_cccccc"), "aaaaaa_bbbbbb_cccccc");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("_a_b_c"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("__a_b_c"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("a_b_c_"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("a_b_c__"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("_a_b_c_"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeSensorNameForGolovan("__a_b_c__"), "a_b_c");
    }
}

} // namespace
