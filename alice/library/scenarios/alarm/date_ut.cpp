#include "date_time.h"

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/timezone_conversion/civil.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NScenarios::NAlarm;
using namespace NDatetime;

using TComponent = TDate::TComponent;

namespace {

TDate CreateWeeksDate(TMaybe<int> weeks, TMaybe<NDatetime::TWeekday> wd = Nothing()) {
    TDate date;
    if (weeks)
        date.Weeks.ConstructInPlace(*weeks, true);
    date.Weekday = wd;
    return date;
};

Y_UNIT_TEST_SUITE(DateTimeUnitTest) {
    Y_UNIT_TEST(Smoke) {
        const auto sample = NSc::TValue::FromJson(R"({
            "years": 2019,
            "months": 3,
            "days": 4
        })");

        const auto date = TDate::FromValue(sample);

        UNIT_ASSERT(date);
        UNIT_ASSERT_VALUES_EQUAL(*date, TDate(2019 /* years */, 3 /* months */, 4 /* days */));
        UNIT_ASSERT(date->Days);
        UNIT_ASSERT(date->HasExactDay());

        const TCivilSecond now{2018, 2, 28, 10, 30, 25};
        UNIT_ASSERT_VALUES_EQUAL(date->Apply(now), TCivilSecond(2019, 3, 4, 10, 30, 25));
    }

    Y_UNIT_TEST(LastDayOfTheMonth) {
        const auto sample = NSc::TValue::FromJson(R"({
            "months": 6,
            "days": 7
        })");
        const auto date = TDate::FromValue(sample);

        UNIT_ASSERT(date);
        UNIT_ASSERT_VALUES_EQUAL(*date, TDate(Nothing() /* years */, TComponent(6, false) /* months */, TComponent(7, false) /* days */));

        const TCivilSecond now{2019, 5, 31, 10, 30, 25};
        UNIT_ASSERT_VALUES_EQUAL(date->Apply(now), TCivilSecond(2019, 6, 7, 10, 30, 25));
    }

    Y_UNIT_TEST(NoMonths) {
        const auto sample = NSc::TValue::FromJson(R"({
            "years": 2018,
            "days": 4
        })");

        const auto date = TDate::FromValue(sample);

        UNIT_ASSERT(date);
        UNIT_ASSERT(date->Days);
        UNIT_ASSERT(date->HasExactDay());
        UNIT_ASSERT_VALUES_EQUAL(
            *date, TDate(TComponent(2018, false) /* years*/, Nothing() /* months */, TComponent(4, false) /* days */));

        const TCivilSecond now{2018, 2, 28, 10, 30, 25};
        UNIT_ASSERT_VALUES_EQUAL(date->Apply(now), TCivilSecond(2018, 1, 4, 10, 30, 25));
    }

    Y_UNIT_TEST(DaysRelative) {
        const auto sample = NSc::TValue::FromJson(R"({
            "years": 2019,
            "days": 4,
            "days_relative": true
        })");

        const auto date = TDate::FromValue(sample);

        UNIT_ASSERT(date);
        UNIT_ASSERT(date->Days);
        UNIT_ASSERT_VALUES_EQUAL(
            *date, TDate(TComponent(2019, false) /* years*/, Nothing() /* months */, TComponent(4, true) /* days */));

        const TCivilSecond now{2018, 3, 28, 10, 30, 25};
        UNIT_ASSERT_VALUES_EQUAL(date->Apply(now), TCivilSecond(2019, 4, 1, 10, 30, 25));
    }

    Y_UNIT_TEST(Tomorrow) {
        const auto sample = NSc::TValue::FromJson(R"({
            "days": 1,
            "days_relative": true
        })");

        const auto date = TDate::FromValue(sample);

        UNIT_ASSERT(date);
        UNIT_ASSERT(date->Days);
        UNIT_ASSERT_VALUES_EQUAL(*date, TDate(Nothing() /* years */, Nothing() /* months */, TComponent(1, true) /* days */));

        const TCivilSecond now{2018, 2, 28, 10, 20, 35};
        UNIT_ASSERT_VALUES_EQUAL(date->Apply(now), TCivilSecond(2018, 3, 1, 10, 20, 35));
    }

    Y_UNIT_TEST(PartRelative) {
        const auto sample = NSc::TValue::FromJson(R"({
            "years": 2018,
            "years_relative": false,
            "months": 3,
            "months_relative": true,
            "days": 4,
            "days_relative": true
        })");

        const auto date = TDate::FromValue(sample);

        UNIT_ASSERT(date);
        UNIT_ASSERT(date->Days);
        UNIT_ASSERT_VALUES_EQUAL(*date,
            TDate(TComponent(2018, false) /* years */, TComponent(3, true) /* months */,
                TComponent(4, true) /* days */));

        const TCivilSecond now{2018, 3, 28, 10, 20, 35};
        UNIT_ASSERT_VALUES_EQUAL(date->Apply(now), TCivilSecond(2018, 7, 2, 10, 20, 35));
    }

    Y_UNIT_TEST(FullRelative) {
        const auto sample = NSc::TValue::FromJson(R"({
            "years": 1,
            "years_relative": true,
            "months": 2,
            "months_relative": true,
            "days": 3,
            "days_relative": true
        })");

        const auto date = TDate::FromValue(sample);

        UNIT_ASSERT(date);
        UNIT_ASSERT(date->Days);
        UNIT_ASSERT_VALUES_EQUAL(*date, TDate(TComponent(1, true) /* years */, TComponent(2, true) /* months */,
            TComponent(3, true) /* days */));

        const TCivilSecond now{2018, 2, 28, 10, 30, 25};
        UNIT_ASSERT_VALUES_EQUAL(date->Apply(now), TCivilSecond(2019, 5, 1, 10, 30, 25));
    }

    Y_UNIT_TEST(DaysFixed) {
        const auto sample = NSc::TValue::FromJson(R"({"days": 1})");
        const auto date = TDate::FromValue(sample);

        UNIT_ASSERT(date);
        UNIT_ASSERT(date->Days);
        UNIT_ASSERT_VALUES_EQUAL(*date,
            TDate(Nothing() /* years */, Nothing() /* months */, TComponent(1, false) /* days */));

        const TCivilSecond now{2018, 2, 28, 10, 30, 25};
        UNIT_ASSERT_VALUES_EQUAL(date->Apply(now), TCivilSecond(2018, 2, 1, 10, 30, 25));
    }

    Y_UNIT_TEST(TodayOrTomorrowSmoke)
    {
        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({
                "days": 0,
                "days_relative": true
            })"));

            UNIT_ASSERT(date);
            UNIT_ASSERT(date->Days);
            UNIT_ASSERT(date->IsTodayOrTomorrow(TCivilSecond{2018, 2, 28, 10, 30, 25}));
            UNIT_ASSERT(date->IsTodayOrTomorrow(TCivilSecond{2018, 3, 1, 10, 30, 25}));
        }

        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({
                "days": 1,
                "days_relative": true
            })"));

            UNIT_ASSERT(date);
            UNIT_ASSERT(date->Days);
            UNIT_ASSERT(date->IsTodayOrTomorrow(TCivilSecond{2018, 2, 28, 10, 30, 25}));
            UNIT_ASSERT(date->IsTodayOrTomorrow(TCivilSecond{2018, 3, 1, 10, 30, 25}));
        }

        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({
                "days": 28,
                "months": 2,
                "years": 2018
            })"));

            UNIT_ASSERT(date);
            UNIT_ASSERT(date->Days);
            UNIT_ASSERT(date->IsTodayOrTomorrow(TCivilSecond{2018, 2, 28, 10, 30, 25}));

            UNIT_ASSERT(!date->IsTodayOrTomorrow(TCivilSecond{2017, 2, 28, 10, 30, 25}));
            UNIT_ASSERT(!date->IsTodayOrTomorrow(TCivilSecond{2018, 3, 1, 10, 30, 25}));
        }

        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({
                "days": 1,
                "months": 3
            })"));

            UNIT_ASSERT(date);
            UNIT_ASSERT(date->Days);
            UNIT_ASSERT(date->IsTodayOrTomorrow(TCivilSecond{2018, 2, 28, 10, 30, 25}));
            UNIT_ASSERT(date->IsTodayOrTomorrow(TCivilSecond{2018, 3, 1, 10, 30, 25}));

            UNIT_ASSERT(!date->IsTodayOrTomorrow(TCivilSecond{2017, 1, 1, 10, 0, 0}));
            UNIT_ASSERT(!date->IsTodayOrTomorrow(TCivilSecond{2018, 2, 27, 10, 0, 0}));
            UNIT_ASSERT(!date->IsTodayOrTomorrow(TCivilSecond{2018, 3, 2, 10, 0, 0}));
        }
    }

    Y_UNIT_TEST(HasExactDay)
    {
        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({ "months": 3 })"));
            UNIT_ASSERT(date);
            UNIT_ASSERT(!date->HasExactDay());
        }

        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({ "years": 3, "month": 3 })"));
            UNIT_ASSERT(date);
            UNIT_ASSERT(!date->HasExactDay());
        }

        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({ "years": 3 })"));
            UNIT_ASSERT(date);
            UNIT_ASSERT(!date->HasExactDay());
        }

        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({ "days": 3 })"));
            UNIT_ASSERT(date);
            UNIT_ASSERT(date->HasExactDay());
        }

        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({ "weekday": 3 })"));
            UNIT_ASSERT(date);
            UNIT_ASSERT(date->HasExactDay());
        }

        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({ "weeks": 3, "weeks_relative": true })"));
            UNIT_ASSERT(date);
            UNIT_ASSERT(date->HasExactDay());
        }
    }

    Y_UNIT_TEST(Weekday)
    {
        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({ "weekday": 0 })"));
            UNIT_ASSERT(!date);
        }

        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({ "weekday": 8 })"));
            UNIT_ASSERT(!date);
        }

        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({
                "weekday": 2
            })"));

            UNIT_ASSERT(date);
            UNIT_ASSERT_VALUES_EQUAL(*date, CreateWeeksDate(Nothing(), NDatetime::TWeekday::tuesday));
            UNIT_ASSERT(date->HasExactDay());
            // 2018, 5, 15 is tuesday
            UNIT_ASSERT_VALUES_EQUAL_C(date->Apply(TCivilSecond{2018, 5, 15, 10, 0, 0}), TCivilSecond(2018, 5, 15, 10, 0, 0), "tuesday becomes tuesday same week");
            UNIT_ASSERT_VALUES_EQUAL_C(date->Apply(TCivilSecond{2018, 5, 14, 10, 0, 0}), TCivilSecond(2018, 5, 15, 10, 0, 0), "monday becomes tuesday same week");
            UNIT_ASSERT_VALUES_EQUAL_C(date->Apply(TCivilSecond{2018, 5, 16, 10, 0, 0}), TCivilSecond(2018, 5, 22, 10, 0, 0), "thursday becomes tuesday next week");
        }
    }

    Y_UNIT_TEST(WeeksSmoke)
    {
        { // Supports only relative weeks(1)
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({ "weeks": 1 })"));
            UNIT_ASSERT(!date);
        }

        { // Supports only relative weeks(2)
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({ "weeks": 1, "weeks_relative": false })"));
            UNIT_ASSERT(!date);
        }

        { // Supports weeks without YMD
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({ "weeks": 1, "weeks_relative": true, "days": 1 })"));
            UNIT_ASSERT(!date);
        }

        { // check for HasExactDay
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({ "weeks": 1, "weeks_relative": true })"));
            UNIT_ASSERT(date);
            UNIT_ASSERT(date->HasExactDay());
        }
    }

    Y_UNIT_TEST(Weeks)
    {
        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({
                "weeks": 1,
                "weeks_relative": true
            })"));

            UNIT_ASSERT(date);
            UNIT_ASSERT_VALUES_EQUAL(*date, CreateWeeksDate(1));
        }

        {
            const auto date = TDate::FromValue(NSc::TValue::FromJson(R"({
                "weeks": 1,
                "weeks_relative": true,
                "weekday": 2
            })"));

            UNIT_ASSERT(date);
            UNIT_ASSERT_VALUES_EQUAL(*date, CreateWeeksDate(1, NDatetime::TWeekday::tuesday));
            // 2018, 5, 15 is tuesday
            UNIT_ASSERT_VALUES_EQUAL_C(date->Apply(TCivilSecond{2018, 5, 15, 18, 46, 5}), TCivilSecond(2018, 5, 22, 18, 46, 5), "tuesday becomes tuesday next week");
            UNIT_ASSERT_VALUES_EQUAL_C(date->Apply(TCivilSecond{2018, 5, 16, 18, 46, 5}), TCivilSecond(2018, 5, 22, 18, 46, 5), "monday becomes tuesday next week");
            UNIT_ASSERT_VALUES_EQUAL_C(date->Apply(TCivilSecond{2018, 5, 14, 18, 46, 5}), TCivilSecond(2018, 5, 22, 18, 46, 5), "thursday becomes tuesday next week");
        }

        {
            NSc::TValue json = NSc::TValue::FromJson(R"({
                "weeks": 1,
                "weeks_relative": true,
            })");

            for (int i = 1; i < 8; ++i) {
                json["weekday"].SetIntNumber(i);
                const auto date{TDate::FromValue(json)};

                UNIT_ASSERT(date);
                UNIT_ASSERT(date->Weeks);
                // 2018, 5, 20 is sunday
                UNIT_ASSERT_VALUES_EQUAL(date->Apply(TCivilSecond{2018, 5, 15, 18, 46, 5}), TCivilSecond(2018, 5, i + 20, 18, 46, 5));
            }
        }
    }
}
} // namespace
