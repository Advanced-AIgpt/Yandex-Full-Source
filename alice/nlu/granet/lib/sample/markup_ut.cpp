#include "markup.h"
#include <library/cpp/testing/unittest/registar.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(TMarkup) {

    Y_UNIT_TEST(TestSimple) {
        {
            const TSampleMarkup expected = ReadSampleMarkup(true, "привет я 'петя'(item_name/custom.address_book.item_name:petya)");

            const TSampleMarkup actual1 = ReadSampleMarkup(true, "привет я 'петя'(item_name/custom.address_book.item_name:petya)");
            UNIT_ASSERT(expected.CheckResult(actual1, true /* compareSlotsByTop */));

            const TSampleMarkup actual2 = ReadSampleMarkup(true, "привет я 'петя'(item_name/string:petya)");
            UNIT_ASSERT(!expected.CheckResult(actual2, true /* compareSlotsByTop */));
        }
        {
            const TSampleMarkup expected = ReadSampleMarkup(true, "привет я 'петя'(item_name:petya)");

            const TSampleMarkup actual1 = ReadSampleMarkup(true, "привет я 'петя'(item_name/custom.address_book.item_name:petya)");
            UNIT_ASSERT(expected.CheckResult(actual1, true /* compareSlotsByTop */));

            const TSampleMarkup actual2 = ReadSampleMarkup(true, "привет я 'петя'(item_name/string:petya)");
            UNIT_ASSERT(expected.CheckResult(actual2, true /* compareSlotsByTop */));

            const TSampleMarkup actual3 = ReadSampleMarkup(true, "привет я 'петя'(item_name/petya2)");
            UNIT_ASSERT(!expected.CheckResult(actual3, true /* compareSlotsByTop */));
        }
        {
            const TSampleMarkup expected = ReadSampleMarkup(true, "привет я 'петя'(item_name)");

            const TSampleMarkup actual1 = ReadSampleMarkup(true, "привет я 'петя'(item_name/custom.address_book.item_name:petya)");
            UNIT_ASSERT(expected.CheckResult(actual1, true /* compareSlotsByTop */));

            const TSampleMarkup actual2 = ReadSampleMarkup(true, "привет я 'петя'(item_name/string:petya)");
            UNIT_ASSERT(expected.CheckResult(actual2, true /* compareSlotsByTop */));

            const TSampleMarkup actual3 = ReadSampleMarkup(true, "привет я 'петя'(item_name/petya2)");
            UNIT_ASSERT(expected.CheckResult(actual3, true /* compareSlotsByTop */));
        }
    }

    Y_UNIT_TEST(TestSeveralValues) {
        {
            const TSampleMarkup expected = ReadSampleMarkup(true, "привет я 'петя'(item_name:petya)");

            const TSampleMarkup actual1 = ReadSampleMarkup(true, "привет я 'петя'(item_name:petya;petya2)");
            UNIT_ASSERT(expected.CheckResult(actual1, true /* compareSlotsByTop */));

            const TSampleMarkup actual2 = ReadSampleMarkup(true, "привет я 'петя'(item_name:petya2)");
            UNIT_ASSERT(!expected.CheckResult(actual2, true /* compareSlotsByTop */));

            const TSampleMarkup actual3 = ReadSampleMarkup(true, "привет я 'петя'(item_name:petya2;petya)");
            UNIT_ASSERT(!expected.CheckResult(actual3, true /* compareSlotsByTop */));

            const TSampleMarkup actual4 = ReadSampleMarkup(true, "привет я 'петя'(item_name:petya2;petya)");
            UNIT_ASSERT(expected.CheckResult(actual4, false /* compareSlotsByTop */));
        }
        {
            const TSampleMarkup expected = ReadSampleMarkup(true, "привет я 'петя'(item_name:petya;petya2)");

            const TSampleMarkup actual1 = ReadSampleMarkup(true, "привет я 'петя'(item_name:petya;petya2)");
            UNIT_ASSERT(expected.CheckResult(actual1, true /* compareSlotsByTop */));

            const TSampleMarkup actual2 = ReadSampleMarkup(true, "привет я 'петя'(item_name:petya;petya2;petya3)");
            UNIT_ASSERT(expected.CheckResult(actual2, true /* compareSlotsByTop */));

            const TSampleMarkup actual3 = ReadSampleMarkup(true, "привет я 'петя'(item_name:petya2)");
            UNIT_ASSERT(!expected.CheckResult(actual3, true /* compareSlotsByTop */));

            const TSampleMarkup actual4 = ReadSampleMarkup(true, "привет я 'петя'(item_name:petya2;petya)");
            UNIT_ASSERT(!expected.CheckResult(actual4, true /* compareSlotsByTop */));

            const TSampleMarkup actual5 = ReadSampleMarkup(true, "привет я 'петя'(item_name:petya2;petya)");
            UNIT_ASSERT(expected.CheckResult(actual5, false /* compareSlotsByTop */));

            const TSampleMarkup actual6 = ReadSampleMarkup(true, "привет я 'петя'(item_name:petya3;petya2;petya)");
            UNIT_ASSERT(expected.CheckResult(actual6, false /* compareSlotsByTop */));
        }

    }

}
