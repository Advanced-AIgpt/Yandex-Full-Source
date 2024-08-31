#include "utils.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NSmallGeo;

Y_UNIT_TEST_SUITE(SmallGeoUtilsTest) {
    Y_UNIT_TEST(RemoveParenthesesTest) {

        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses("abcde"), "abcde");
        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses("Гонконг (Сянган)"), "Гонконг");
        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses("Гонконг(Сянган)"), "Гонконг");
        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses("Гонконг\n(Сянган)"), "Гонконг");
        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses("Гонконг (Сянган) Китай"), "Гонконг (Сянган) Китай");
        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses("  Гонконг  (Сянган)"), "Гонконг");
        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses("\nГонконг\n(Сянган)"), "Гонконг");
        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses("\nГонконг\n(Сянган)"), "Гонконг");
        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses("(abc"), "(abc");
        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses("abc)"), "abc)");
        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses("(abc)"), "(abc)");
        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses("(ab(c)"), "(ab(c)");
        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses("ab(c))"), "ab(c))");
        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses("a(b(c)"), "a");
        UNIT_ASSERT_VALUES_EQUAL(RemoveParentheses(""), "");
    }
}
