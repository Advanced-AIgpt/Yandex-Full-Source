#include "common.h"

#include <alice/nlu/libs/fst/fst_date_time_range.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NTestHelpers;

Y_UNIT_TEST_SUITE(TFstDateTimeRangeTests) {

    static const TTestCaseRunner<TFstDateTimeRange>& T() {
        static const TTestCaseRunner<TFstDateTimeRange> T{"DATETIME_RANGE", LANG_RUS};
        return T;
    }

    static TEntity CE(size_t begin, size_t end, TStringBuf json) {
        return CreateEntity(begin, end, "DATETIME_RANGE", NSc::TValue::FromJsonThrow(json));
    }

    Y_UNIT_TEST(ParseEmpty) {
        const TTestCase testCases[] = {
            {"", {}}
        };
        T().Run(testCases);
    }


    Y_UNIT_TEST(FromToDate) {
        const auto result = R"({"start": {"months": 12, "days": 15}, "end": {"months": 12, "days": 25}})";
        const TTestCase testCases[] = {
            {"погода с 15 по 25 декабря", {CE(1, 6, result)}},
            {"погода с 15 декабря по 25 декабря в москве", {CE(1, 7, result)}},
            {"погода от 15 до 25 декабря", {CE(1, 6, result)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(FromToDateNoMonth) {
        const auto result = R"({"start": {"days": 15}, "end": {"days": 25}})";
        const TTestCase testCases[] = {
            {"погода с 15 по 25", {CE(1, 5, result)}},
            {"погода с 15 по 25 число", {CE(1, 6, result)}},
            {"погода с пятнадцатого по двадцать пятое", {CE(1, 5, result)}},
            {"погода с пятнадцатого по двадцать пятое число", {CE(1, 6, result)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(FromToDateDifferentMonths) {
        const TTestCase testCases[] = {
            {"погода с 10 ноября по 15 декабря",
             {CE(1, 7, R"({"start": {"months": 11, "days": 10}, "end": {"months": 12, "days": 15}})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(FromToDateDifferentMonthsAndYear) {
        const TTestCase testCases[] = {
            {"погода с 25 декабря до 10 января",
             {CE(1, 7, R"({"start": {"months": 12, "days": 25},
                           "end": {"months": 1, "days": 10, "years": 1, "years_relative": true}})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(OnNextNDays) {
        const TTestCase testCases[] = {
            {"погода на 3 дня",
             {CE(2, 4, R"({"start": {"days_relative": true, "days": 0}, "end": {"days_relative": true, "days": 3}})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(OnNextTime) {
        const TTestCase testCases[] = {
            {"расписание на следующие 3 часа",
             {CE(2, 5, R"({"start": {"hours": 0, "hours_relative": true},
                           "end": {"hours": 3, "hours_relative": true}})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(OnNextNCombined) {
        const TTestCase testCases[] = {
            {"погода на ближайшие полторы недели",
             {CE(2, 5, R"({"start": {"days": 0, "days_relative": true, "weeks": 0, "weeks_relative": true},
                           "end": {"days": 3, "days_relative": true, "weeks": 1, "weeks_relative": true}})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(OnNextDay) {
        const auto result = TVector<NSc::TValue>{NSc::TValue::FromJsonThrow(R"({"start": {"days_relative": true, "days": 0}, "end": {"days_relative": true, "days": 1}})")};
        const TTestCaseValue testCases[] = {
            {"погода на день", result},
            {"погода на один день", result},
            {"погода на этот день", result}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(OnCurrentWeek) {
        const auto result = TVector<NSc::TValue>{NSc::TValue::FromJsonThrow(
                R"({"start": {"weeks_relative": true, "weeks": 0}, "end": {"weeks_relative": true, "weeks": 1}})")};
        const TTestCaseValue testCases[] = {
            {"погода на неделю", result},
            {"погода на одну неделю", result},
            {"погода на эту неделю", result}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(OnNextWeek) {
        const auto result = TVector<NSc::TValue>{NSc::TValue::FromJsonThrow(R"({
            "start": {"weeks_relative": true, "weeks": 1},
            "end": {"weeks_relative": true, "weeks": 2}
        })")};
        const TTestCaseValue testCases[] = {
            {"погода на следующую неделю", result},
            {"погода на следующей неделе", result},
            {"погода на ту неделю", result}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(OnTodayTomorrow) {
        const auto result = TVector<NSc::TValue>{NSc::TValue::FromJsonThrow(R"({
            "start": {"days_relative": true, "days": 0},
            "end": {"days_relative": true, "days": 2}
        })")};
        const TTestCaseValue testCases[] = {
            {"погода на сегодня-завтра", result},
            {"погода на сегодня завтра", result},
            {"погода на сегодня и завтра", result},
            {"погода на сегодня и на завтра", result},
            {"погода на ближайшие пару дней", result}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(OnTodayTomorrowAfterTomorrow) {
        const auto result = TVector<NSc::TValue>{NSc::TValue::FromJsonThrow(R"({
            "start": {"days_relative": true, "days": 0},
            "end": {"days_relative": true, "days": 3}
        })")};
        const TTestCaseValue testCases[] = {
            {"погода на сегодня, завтра и послезавтра", result},
            {"погода на сегодня, на завтра и на послезавтра", result},
            {"погода на сегодня завтра послезавтра", result}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(OnWeekend) {
        const auto result = TVector<NSc::TValue>{NSc::TValue::FromJsonThrow(R"({
            "start": {"weekend": true, "weeks": 0, "weeks_relative": true},
            "end": {"weekend": true, "weeks": 0, "weeks_relative": true}
        })")};
        const TTestCaseValue testCases[] = {
            {"погода на выходных", result},
            {"погода в выходные", result},
            {"погода на этих выходных", result},
            {"погода на уикенд", result},
            {"погода на викенде", result}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(NextWeekend) {
        const auto result = TVector<NSc::TValue>{NSc::TValue::FromJsonThrow(R"({
            "start": {"weekend": true, "weeks": 1, "weeks_relative": true},
            "end": {"weekend": true, "weeks": 1, "weeks_relative": true}
        })")};
        const TTestCaseValue testCases[] = {
            {"погода на следующих выходных", result},
            {"погода в следующие выходные", result},
            {"погода на следующий уикенд", result},
            {"погода на следующем викенде", result}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(OnHolidays) {
        const auto result = TVector<NSc::TValue>{NSc::TValue::FromJsonThrow(R"({
            "start": {"holidays": true},
            "end": {"holidays": true}
        })")};
        const TTestCaseValue testCases[] = {
            {"погода на праздниках", result},
            {"погода на ближайших праздниках", result},
            {"погода в ближайшие праздники", result}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(FromToWeekday) {
        const auto result = TVector<NSc::TValue>{NSc::TValue::FromJsonThrow(R"({
            "start": {"weekday": 4},
            "end": {"weekday": 6}
        })")};
        const TTestCaseValue testCases[] = {
            {"погода с четверга по субботу", result},
            {"погода с этого четверга по субботу", result},
            {"погода с этого четверга по ближайшую субботу", result},
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(FromToWeekdayNextWeek) {
        const auto result = TVector<NSc::TValue>{NSc::TValue::FromJsonThrow(R"({
            "start": {"weekday": 4},
            "end": {"weekday": 1, "weeks": 1, "weeks_relative": true}
        })")};
        const TTestCaseValue testCases[] = {
            {"погода с четверга по понедельник", result},
            {"погода с четверга по следующий понедельник", result}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(UpToWeekday) {
        const auto result = TVector<NSc::TValue>{NSc::TValue::FromJsonThrow(R"({
            "start": {"days_relative": true, "days": 0},
            "end": {"weekday": 5}
        })")};
        const TTestCaseValue testCases[] = {
            {"погода до пятницы", result}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(WeekdayRange) {
        const TTestCaseValue testCases[] = {
            {"погода со вторника по среду",
             {NSc::TValue::FromJsonThrow(R"({"start": {"weekday": 2}, "end": {"weekday": 3}})")}},
            {"погода со среды по четверг",
             {NSc::TValue::FromJsonThrow(R"({"start": {"weekday": 3}, "end": {"weekday": 4}})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(FromToMonthDays) {
        const TTestCaseValue testCases[] = {
            {"погода на 15 16 мая",
             {NSc::TValue::FromJsonThrow(R"({"start": {"days": 15, "months": 5}, "end": {"days": 16, "months": 5}})")}},
            {"погода на 15 20 мая",
             {NSc::TValue::FromJsonThrow(R"({"start": {"days": 15, "months": 5}, "end": {"days": 20, "months": 5}})")}},
            {"погода на 15-16 мая",
             {NSc::TValue::FromJsonThrow(R"({"start": {"days": 15, "months": 5}, "end": {"days": 16, "months": 5}})")}},
            {"погода на 15 - 16 мая",
             {NSc::TValue::FromJsonThrow(R"({"start": {"days": 15, "months": 5}, "end": {"days": 16, "months": 5}})")}},
            {"погода на 15 и 16 мая",
             {NSc::TValue::FromJsonThrow(R"({"start": {"days": 15, "months": 5}, "end": {"days": 16, "months": 5}})")}},
            {"погода на 15 35 мая", {}}
        };
        T().Run(testCases);
    }

} // test suite
