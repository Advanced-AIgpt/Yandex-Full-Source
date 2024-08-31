#include "common.h"

#include <alice/nlu/libs/fst/fst_date_time.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NTestHelpers;

Y_UNIT_TEST_SUITE(TFstDateTimeTrTests) {

    static const TTestCaseRunner<TFstDateTime>& T() {
        static const TTestCaseRunner<TFstDateTime> T{"DATETIME", LANG_TUR};
        return T;
    }

    static TEntity CE(size_t begin, size_t end, TStringBuf json) {
        return CreateEntity(begin, end, "DATETIME", NSc::TValue::FromJsonThrow(json));
    }

    Y_UNIT_TEST(ParseEmpty) {
        const TTestCase testCases[] = {
            {"", {}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeSpecDay) {
        const TTestCase testCases[] = {
            {"bugün hava nedir", {CE(0, 1, R"({"days": 0, "days_relative": true})")}},
            {"yarın hava nedir", {CE(0, 1, R"({"days": 1, "days_relative": true})")}},
            {"yarından sonraki gün hava ne olacak", {CE(0, 3, R"({"days": 2, "days_relative": true})")}},
            {"dün hava nasıldı", {CE(0, 1, R"({"days": -1, "days_relative": true})")}},
            {"önceki gün hava nasıldı", {CE(0, 2, R"({"days": -2, "days_relative": true})")}},
            {"şimdi hava nedir", {CE(0, 1, R"({"seconds":0,"seconds_relative":true})")}}
        };
        T().Run(testCases);
    }

} // test suite
