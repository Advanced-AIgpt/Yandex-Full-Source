#include "common.h"

#include <alice/nlu/libs/fst/fst_units.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NTestHelpers;

Y_UNIT_TEST_SUITE(TFstUnitsTests) {

    static const TTestCaseRunner<TFstUnits>& T() {
        static const TTestCaseRunner<TFstUnits> T{"UNITS_TIME", LANG_RUS};
        return T;
    }

    static TEntity CE(size_t begin, size_t end, TStringBuf json) {
        return CreateEntity(begin, end, "UNITS_TIME", NSc::TValue::FromJsonThrow(json));
    }

    Y_UNIT_TEST(ParseEmpty) {
        const TTestCase testCases[] = {
            {"", {}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(UnitsTime) {
        const TTestCase testCases[] = {
            {"минута", {CE(0, 1, R"({"minutes": 1})")}},
            {"через минуту", {CE(1, 2, R"({"minutes": 1})")}},
            {"час", {CE(0, 1, R"({"hours": 1})")}},
            {"полминуты", {CE(0, 1, R"({"minutes": 0.5})")}},
            {"полторы минуты", {CE(0, 2, R"({"minutes": 1.5})")}},
            {"сто двадцать три минуты", {CE(0, 2, R"({"minutes": 123})")}},
            {"сто минут сорок секунд", {CE(0, 4, R"({"minutes": 100, "seconds": 40})")}},
            {"сто минут и сорок секунд", {CE(0, 5, R"({"minutes": 100, "seconds": 40})")}},
            {"на пару минут", {CE(1, 3, R"({"minutes": 2})")}},
            {"на 5 30", {CE(1, 3, R"({"minutes": 5, "seconds": 30})")}},
            {"на 120", {CE(1, 2, R"({"minutes": 120})")}},
            {"вырази в минутах 23/25 часа", {CE(3, 7, R"({"hours": 0.92})")}},
            {"перемотай на час двадцать", {CE(2, 4, R"({"hours": 1, "minutes": 20})")}},
            {"засеки минуту сорок", {CE(1, 3, R"({"minutes": 1, "seconds": 40})")}}
        };
        T().Run(testCases);
    }

} // test suite
