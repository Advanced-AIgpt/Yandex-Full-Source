#include "utils.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NMisspell;

namespace {

Y_UNIT_TEST_SUITE(Misspell) {
    Y_UNIT_TEST(CleanMisspellMarkup) {
        auto res = CleanMisspellMarkup("ба­(л)­ерина");
        UNIT_ASSERT_VALUES_EQUAL(res.Defined(), true);
        UNIT_ASSERT_VALUES_EQUAL(*res, "балерина");

        res = CleanMisspellMarkup("пр­(и)­вет,м­(о)­сква...");
        UNIT_ASSERT_VALUES_EQUAL(res.Defined(), true);
        UNIT_ASSERT_VALUES_EQUAL(*res, "привет,москва...");

        res = CleanMisspellMarkup("мер­(с)­едес");
        UNIT_ASSERT_VALUES_EQUAL(res.Defined(), true);
        UNIT_ASSERT_VALUES_EQUAL(*res, "мерседес");

        res = CleanMisspellMarkup("дауга­(в)­пилс");
        UNIT_ASSERT_VALUES_EQUAL(res.Defined(), true);
        UNIT_ASSERT_VALUES_EQUAL(*res, "даугавпилс");

        res = CleanMisspellMarkup("рубли в до­(лл)­ары");
        UNIT_ASSERT_VALUES_EQUAL(res.Defined(), true);
        UNIT_ASSERT_VALUES_EQUAL(*res, "рубли в доллары");
    }
}

} // namespace
