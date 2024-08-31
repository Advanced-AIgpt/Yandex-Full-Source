#include "http_request.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TestHttpRequestSuite) {
    Y_UNIT_TEST(TestReqId) {
        UNIT_ASSERT_VALUES_EQUAL(true, CheckBalancerReqid("1517974491920783-6989156297992730115"));
        UNIT_ASSERT_VALUES_EQUAL(false, CheckBalancerReqid("6989156297992730115"));
        UNIT_ASSERT_VALUES_EQUAL(false, CheckBalancerReqid("xxx-111"));
        UNIT_ASSERT_VALUES_EQUAL(false, CheckBalancerReqid("1-1-1"));
        UNIT_ASSERT_VALUES_EQUAL(false, CheckBalancerReqid("123  -   456"));
        UNIT_ASSERT_VALUES_EQUAL(false, CheckBalancerReqid("123-"));
        UNIT_ASSERT_VALUES_EQUAL(false, CheckBalancerReqid("-456"));
        UNIT_ASSERT_VALUES_EQUAL(false, CheckBalancerReqid("-123 -456"));
        UNIT_ASSERT_VALUES_EQUAL(false, CheckBalancerReqid("123 456"));
    }
};

