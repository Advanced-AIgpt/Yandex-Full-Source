#include "wildcards.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {
Y_UNIT_TEST_SUITE(Wildcards) {
    Y_UNIT_TEST(Smoke) {
        UNIT_ASSERT(Matches(TWildcard(""), ""));
        UNIT_ASSERT(!Matches(TWildcard(""), "a"));
        UNIT_ASSERT(!Matches(TWildcard("a"), ""));
        UNIT_ASSERT(Matches(TWildcard("a"), "a"));

        UNIT_ASSERT(Matches(TWildcard("*"), "hello, world!"));
        UNIT_ASSERT(Matches(TWildcard("hello*"), "hello, world!"));
        UNIT_ASSERT(Matches(TWildcard("*world*"), "hello, world!"));
        UNIT_ASSERT(!Matches(TWildcard("*world*?"), "hello, world!"));

        UNIT_ASSERT(Matches(TWildcard("hello, world!"), "hello, world!"));
        UNIT_ASSERT(!Matches(TWildcard("hola mundo!"), "hello, world!"));

        UNIT_ASSERT(Matches(TWildcard("******"), "a"));
        UNIT_ASSERT(Matches(TWildcard("**"), ""));
    }
}
}  // namespace
