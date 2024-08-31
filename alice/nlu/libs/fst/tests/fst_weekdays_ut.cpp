#include "common.h"

#include <alice/nlu/libs/fst/fst_weekdays.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NTestHelpers;

Y_UNIT_TEST_SUITE(TFstWeekdaysTests) {

    static const TTestCaseRunner<TFstWeekdays>& T() {
        static const TTestCaseRunner<TFstWeekdays> T{"WEEKDAYS", LANG_RUS};
        return T;
    }

    static TEntity CE(size_t begin, size_t end, TStringBuf json) {
        return CreateEntity(begin, end, "WEEKDAYS", NSc::TValue::FromJsonThrow(json));
    }

    Y_UNIT_TEST(ParseEmpty) {
        const TTestCase testCases[] = {
            {"", {}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(Test) {
        const TTestCase testCases[] = {
            {"поставь будильник в понедельник на 7 часов", {CE(2, 4, R"({"weekdays": [1], "repeat": false})")}},
            {"поставь будильник с понедельника по среду на 7 часов", {CE(2, 6, R"({"weekdays": [1, 2, 3], "repeat": false})")}},
            {"поставь будильник по пятницам на 7 часов", {CE(2, 4, R"({"weekdays": [5], "repeat": true})")}},
            {"поставь будильник на четверг на 7 часов", {CE(2, 4, R"({"weekdays": [4], "repeat": false})")}},
            {"на будни поставь будильник в 10", {CE(0, 2, R"({"weekdays": [1, 2, 3, 4, 5], "repeat": false})")}},
            {"разбуди в 10 по будням", {CE(3, 5, R"({"weekdays": [1, 2, 3, 4, 5], "repeat": true})")}},
            {"по выходным будильник на 10", {CE(0, 2, R"({"weekdays": [6, 7], "repeat": true})")}},
            {"в выходные будет хорошая погода", {CE(0, 2, R"({"weekdays": [6, 7], "repeat": false})")}},
            {"поставь будильники на вторники на 7 часов", {CE(2, 4, R"({"weekdays": [2], "repeat": true})")}},
            {"в понедельник и вторник напомни зайти к врачу", {CE(0, 4, R"({"weekdays": [1, 2], "repeat": false})")}},
            {"в понедельник четверг и вторник разбуди в 8", {CE(0, 5, R"({"weekdays": [1, 2, 4], "repeat": false})")}},
            {"по понедельникам средам и пятницам у меня английский", {CE(0, 5, R"({"weekdays": [1, 3, 5], "repeat": true})")}},
            {"каждое воскресенье я хожу в баню", {CE(0, 2, R"({"weekdays": [7], "repeat": true})")}},
            {"каждые выходные напоминай проснуться", {CE(0, 2, R"({"weekdays": [6, 7], "repeat": true})")}},
            {"с понедельника начинаю новую жизнь", {CE(0, 2, R"({"weekdays": [1, 2, 3, 4, 5, 6, 7], "repeat": false})")}},
            {"с пятницы по вторник я буду недоступен", {CE(0, 4, R"({"weekdays": [1, 2, 5, 6, 7], "repeat": false})")}},
            {"я буду недоступен со среды по среду", {CE(3, 7, R"({"weekdays": [1, 2, 3, 4, 5, 6, 7], "repeat": false})")}},
            {"каждый день в семь часов", {CE(0, 2, R"({"weekdays": [1, 2, 3, 4, 5, 6, 7], "repeat": true})")}}
        };
        T().Run(testCases);
    }

} // test suite
