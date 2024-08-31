#include <alice/library/util/charchecker.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

using namespace NAlice;

Y_UNIT_TEST_SUITE(CharChecker) {

Y_UNIT_TEST(ValidUuid)
{
    UNIT_ASSERT(CheckUuid("01234567-89ab-cdef-1234-567890ABCDEF"));
    UNIT_ASSERT(CheckUuid("0123456789ab-cdef-1234-567890ABCDEF"));
    UNIT_ASSERT(CheckUuid("01234567-89abcdef-1234-567890ABCDEF"));
    UNIT_ASSERT(CheckUuid("01234567-89ab-cdef1234-567890ABCDEF"));
    UNIT_ASSERT(CheckUuid("01234567-89ab-cdef-1234567890ABCDEF"));
    UNIT_ASSERT(CheckUuid("0123456789abcdef1234567890ABCDEF"));
}

Y_UNIT_TEST(InvalidUuid)
{
    UNIT_ASSERT(!CheckUuid("01234567-89ab-cdef-123-4567890ABCDEF"));    // wrong place for a dash
    UNIT_ASSERT(!CheckUuid("01234567-89a-cdef-1234-567890ABCDEF"));     // lack of HEX digit
    UNIT_ASSERT(!CheckUuid("01234567-89ab-cdef-1234-567890zBCDEF"));    // not a HEX digit
    UNIT_ASSERT(!CheckUuid("01234567 89ab-cdef-1234-567890ABCDEF"));    // not a dash
    UNIT_ASSERT(!CheckUuid("01234567-89ab-cd-ef-1234-567890ABCDEF"));   // extra dash
    UNIT_ASSERT(!CheckUuid("01234567-89ab-cdef-1234-567890ABCDEF0"));   // too long
    UNIT_ASSERT(!CheckUuid("0123456789abcdef1234567890ABCDEF0"));       // too long
    UNIT_ASSERT(!CheckUuid(""));                                        // empty string
}

} // namespace NAlice
