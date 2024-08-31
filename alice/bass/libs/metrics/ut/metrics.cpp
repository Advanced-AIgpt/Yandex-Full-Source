#include <alice/bass/libs/metrics/metrics.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NMonitoring;

TString F(TStringBuf name) {
    return NormalizeMetricNameForGolovan(name);
}

Y_UNIT_TEST_SUITE(BassMetricsTest) {
    Y_UNIT_TEST(TestNormalizeMetricNameForGolovan) {

        UNIT_ASSERT_VALUES_EQUAL(F(""), "");
        UNIT_ASSERT_VALUES_EQUAL(F("abc"), "abc");
        UNIT_ASSERT_VALUES_EQUAL(F("a_b_c"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(F("a__b_c"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(F("a_b__c"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(F("a_b__c"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(F("a______b______c"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(F("aaa_bbb_ccc"), "aaa_bbb_ccc");
        UNIT_ASSERT_VALUES_EQUAL(F("aaaaaa_bbbbbb_cccccc"), "aaaaaa_bbbbbb_cccccc");
        UNIT_ASSERT_VALUES_EQUAL(F("_a_b_c"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(F("__a_b_c"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(F("a_b_c_"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(F("a_b_c__"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(F("_a_b_c_"), "a_b_c");
        UNIT_ASSERT_VALUES_EQUAL(F("__a_b_c__"), "a_b_c");
    }
}
