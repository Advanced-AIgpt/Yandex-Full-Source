#include "variadic_format.h"
#include <library/cpp/testing/unittest/registar.h>

namespace NGranet {

Y_UNIT_TEST_SUITE(VariadicFormat) {

    Y_UNIT_TEST(Test) {
        UNIT_ASSERT_EQUAL(VariadicFormat(""), "");
        UNIT_ASSERT_EQUAL(VariadicFormat("a"), "a");
        UNIT_ASSERT_EQUAL(VariadicFormat("a {0}"), "a {0}");
        UNIT_ASSERT_EQUAL(VariadicFormat("a {0}", 1), "a 1");
        UNIT_ASSERT_EQUAL(VariadicFormat("{0}{0}", 123), "123123");
        UNIT_ASSERT_EQUAL(VariadicFormat("{0}", 'c'), "c");
        UNIT_ASSERT_EQUAL(VariadicFormat("{0} b", TStringBuf("abc")), "abc b");
        UNIT_ASSERT_EQUAL(VariadicFormat("a {0} b {1} c {2} d", "A", 1, 1.5), "a A b 1 c 1.5 d");
        UNIT_ASSERT_EQUAL(VariadicFormat("a {2} b {1} c {0} d", "A", 1, 1.5), "a 1.5 b 1 c A d");
        UNIT_ASSERT_EQUAL(VariadicFormat("a {0} b {0} c {0} d", "A", 1, 1.5), "a A b A c A d");
        UNIT_ASSERT_EQUAL(VariadicFormat("a {2} b {2} c {2} d", "A", 1, 1.5), "a 1.5 b 1.5 c 1.5 d");
    }
}

} // namespace NGranet
