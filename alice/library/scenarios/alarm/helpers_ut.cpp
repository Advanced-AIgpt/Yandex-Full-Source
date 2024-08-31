#include "helpers.h"

#include "date_time.h"
#include "weekdays.h"

#include <library/cpp/timezone_conversion/civil.h>
#include <library/cpp/timezone_conversion/convert.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/maybe.h>
#include <util/generic/vector.h>

using namespace NAlice::NScenarios::NAlarm;
using namespace NDatetime;

using TComponent = TDateTimeComponent;

namespace {
constexpr int EPOCH = 1519723230;

// 2018-02-27 12:20:30 Moscow Time, Tuesday
const auto TIME = TInstant::Seconds(EPOCH);

const NDatetime::TTimeZone MoscowTZ() {
    return NDatetime::GetTimeZone("Europe/Moscow");
}

template <typename... TArgs>
TInstant MoscowTime(TArgs&&... args) {
    const NDatetime::TCivilSecond now(std::forward<TArgs>(args)...);
    const auto tz = MoscowTZ();
    return NDatetime::Convert(now, tz);
}

constexpr TStringBuf SINGLE_SHOT_ALARM = "BEGIN:VCALENDAR\r\n"
                                                     "VERSION:2.0\r\n"
                                                     "PRODID:-//Yandex LTD//NONSGML Quasar//EN\r\n"
                                                     "BEGIN:VEVENT\r\n"
                                                     "DTSTART:20180227T092030Z\r\n"
                                                     "DTEND:20180227T092030Z\r\n"
                                                     "BEGIN:VALARM\r\n"
                                                     "TRIGGER;VALUE=DATE-TIME:20180227T092030Z\r\n"
                                                     "ACTION:AUDIO\r\n"
                                                     "END:VALARM\r\n"
                                                     "END:VEVENT\r\n"
                                                     "END:VCALENDAR\r\n";

constexpr TStringBuf SINGLE_WEEK_WEEKDAYS_ALARM =
                "BEGIN:VCALENDAR\r\n"
                "VERSION:2.0\r\n"
                "PRODID:-//Yandex LTD//NONSGML Quasar//EN\r\n"
                "BEGIN:VEVENT\r\n"
                "DTSTART:20180227T092030Z\r\n"
                "DTEND:20180227T092030Z\r\n"
                "RRULE:FREQ=WEEKLY;UNTIL=20180306T092030Z;BYDAY=MO,TU,WE\r\n"
                "BEGIN:VALARM\r\n"
                "TRIGGER:P0D\r\n"
                "ACTION:AUDIO\r\n"
                "END:VALARM\r\n"
                "END:VEVENT\r\n"
                "END:VCALENDAR\r\n";

constexpr TStringBuf REPEATED_WEEKENDS_ALARM = "BEGIN:VCALENDAR\r\n"
                                                           "VERSION:2.0\r\n"
                                                           "PRODID:-//Yandex LTD//NONSGML Quasar//EN\r\n"
                                                           "BEGIN:VEVENT\r\n"
                                                           "DTSTART:20180303T073000Z\r\n"
                                                           "DTEND:20180303T073000Z\r\n"
                                                           "RRULE:FREQ=WEEKLY;BYDAY=SA,SU\r\n"
                                                           "BEGIN:VALARM\r\n"
                                                           "TRIGGER:P0D\r\n"
                                                           "ACTION:AUDIO\r\n"
                                                           "END:VALARM\r\n"
                                                           "END:VEVENT\r\n"
                                                           "END:VCALENDAR\r\n";

Y_UNIT_TEST_SUITE(Helpers) {
    Y_UNIT_TEST(SameDayTimeSmoke) {
        const NDatetime::TCivilSecond a{2018, 1, 1, 10, 30, 15};
        const NDatetime::TCivilSecond b{2017, 2, 3, 10, 30, 15};
        const NDatetime::TCivilSecond c{2018, 1, 1, 10, 30, 16};

        UNIT_ASSERT(SameDayTime(a, b));
        UNIT_ASSERT(!SameDayTime(a, c));
        UNIT_ASSERT(!SameDayTime(b, c));
    }

    Y_UNIT_TEST(SameDateSmoke) {
        const NDatetime::TCivilSecond a{2018, 2, 7, 10, 30, 15};
        const NDatetime::TCivilSecond b{2017, 2, 3, 10, 30, 15};
        const NDatetime::TCivilSecond c{2018, 3, 7, 10, 30, 16};
        const NDatetime::TCivilSecond d{2018, 2, 7, 20, 14, 53};

        UNIT_ASSERT(!SameDate(a, b));
        UNIT_ASSERT(!SameDate(a, c));
        UNIT_ASSERT(SameDate(a, d));
    }

    Y_UNIT_TEST(RemoveByIndicesSmoke) {
        {
            const TVector<int> original;
            const TVector<size_t> indices;

            UNIT_ASSERT_VALUES_EQUAL(RemoveByIndices(original, indices), TVector<int>());
        }

        {
            const TVector<int> original = {{1, 2, 3, 4, 5}};
            const TVector<size_t> indices;
            UNIT_ASSERT_VALUES_EQUAL(RemoveByIndices(original, indices), original);
        }

        {
            const TVector<int> original = {{1, 2, 3, 4, 5}};
            const TVector<size_t> indices(1, 0);

            const TVector<int> expected = {{2, 3, 4, 5}};
            UNIT_ASSERT_VALUES_EQUAL(RemoveByIndices(original, indices), expected);
        }

        {
            const TVector<int> original = {{1, 2, 3, 4, 5}};
            const TVector<size_t> indices(1, 4);

            const TVector<int> expected = {{1, 2, 3, 4}};
            UNIT_ASSERT_VALUES_EQUAL(RemoveByIndices(original, indices), expected);
        }

        {
            const TVector<int> original = {{1, 2, 3, 4, 5}};
            const TVector<size_t> indices = {{1, 2, 4}};

            const TVector<int> expected = {{1, 4}};
            UNIT_ASSERT_VALUES_EQUAL(RemoveByIndices(original, indices), expected);
        }

        {
            const TVector<int> original = {{1, 2, 3, 4, 5}};
            const TVector<size_t> indices = {{0, 1, 2, 3, 4}};

            const TVector<int> expected;
            UNIT_ASSERT_VALUES_EQUAL(RemoveByIndices(original, indices), expected);
        }

        {
            const TVector<int> original = {{1, 2, 3, 4, 5}};
            const TVector<size_t> indices(1, 10);

            UNIT_ASSERT_EXCEPTION_CONTAINS(RemoveByIndices(original, indices), yexception, "Invalid indices");
        }

        {
            const TVector<int> original = {{1, 2, 3, 4, 5}};
            const TVector<size_t> indices = {{3, 2, 1}};

            UNIT_ASSERT_EXCEPTION_CONTAINS(RemoveByIndices(original, indices), yexception, "Invalid indices");
        }

        {
            const TVector<int> original = {{1, 2, 3, 4, 5}};
            const TVector<size_t> indices = {{2, 3, 1}};

            UNIT_ASSERT_EXCEPTION_CONTAINS(RemoveByIndices(original, indices), yexception, "Invalid indices");
        }
    }

    Y_UNIT_TEST(IncludesSmoke) {
        const TVector<int> a = {{1, 2, 3, 3, 4}};
        const TVector<int> b;
        const TVector<int> c = {{1, 1, 1}};
        const TVector<int> d = {{1, 1, 1, 1}};
        const TVector<int> e = {{3, 4, 5}};

        UNIT_ASSERT(Includes(a, b));
        UNIT_ASSERT(!Includes(b, a));

        UNIT_ASSERT(Includes(a, c));
        UNIT_ASSERT(!Includes(c, a));

        UNIT_ASSERT(Includes(a, d));
        UNIT_ASSERT(!Includes(d, a));

        UNIT_ASSERT(Includes(c, d));
        UNIT_ASSERT(Includes(d, c));

        UNIT_ASSERT(!Includes(a, e));
        UNIT_ASSERT(!Includes(e, a));
    }
}

Y_UNIT_TEST_SUITE(HelpersTime) {
    Y_UNIT_TEST(DayTimeSmoke) {
        {
            const TCivilSecond now{2018, 3, 1, 10, 0, 0};
            const TDayTime dayTime(Nothing() /* hours */, TComponent(10, false) /* minutes */, Nothing() /* seconds */,
                                   TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(GetEventTime(now, dayTime), TCivilSecond(2018, 3, 2, 0, 10, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmTime(now, dayTime), TCivilSecond(2018, 3, 2, 0, 10, 0));
        }

        {
            const TCivilSecond now{2018, 3, 1, 10, 0, 0};
            const TDayTime dayTime(TComponent(11, false) /* hours */, Nothing() /* minutes */, Nothing() /* seconds */,
                                   TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(GetEventTime(now, dayTime), TCivilSecond(2018, 3, 1, 11, 0, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmTime(now, dayTime), TCivilSecond(2018, 3, 1, 11, 0, 0));
        }

        {
            const TCivilSecond now{2018, 3, 1, 11, 0, 0};
            const TDayTime dayTime(TComponent(10, false) /* hours */, Nothing() /* minutes */, Nothing() /* seconds */,
                                   TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(GetEventTime(now, dayTime), TCivilSecond(2018, 3, 1, 22, 0, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmTime(now, dayTime), TCivilSecond(2018, 3, 2, 10, 0, 0));
        }

        {
            const TCivilSecond now{2018, 3, 1, 12, 10, 0};
            const TDayTime dayTime(TComponent(12, false) /* hours */, TComponent(10, false) /* minutes */,
                                   Nothing() /* seconds */, TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(GetEventTime(now, dayTime), TCivilSecond(2018, 3, 2, 0, 10, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmTime(now, dayTime), TCivilSecond(2018, 3, 2, 12, 10, 0));
        }

        {
            const TCivilSecond now{2018, 3, 1, 12, 10, 0};
            const TDayTime dayTime(TComponent(8, true) /* hours */, Nothing() /* minutes */, Nothing() /* seconds */,
                                   TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(GetEventTime(now, dayTime), TCivilSecond(2018, 3, 1, 20, 10, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmTime(now, dayTime), TCivilSecond(2018, 3, 1, 20, 10, 0));
        }
    }

    Y_UNIT_TEST(DateTimeSmoke) {
        const TDate yesterday(Nothing() /* years */, Nothing() /* months */, TComponent(-1, true) /* days */);
        const TDate today(Nothing() /* years */, Nothing() /* months */, TComponent(0, true) /* days */);
        const TDate tomorrow(Nothing() /* years */, Nothing() /* months */, TComponent(1, true) /* days */);

        {
            const TCivilSecond now{2018, 3, 21, 10, 0, 0};
            const TDayTime dayTime(TComponent(9, false) /* hours */, Nothing() /* minutes */, Nothing() /* seconds */,
                                   TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(GetEventDateTime(now, dayTime, tomorrow), TCivilSecond(2018, 3, 22, 9, 0, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmDateTime(now, dayTime, tomorrow), TCivilSecond(2018, 3, 22, 9, 0, 0));
        }

        {
            const TCivilSecond now{2018, 3, 21, 10, 0, 0};
            const TDayTime dayTime(TComponent(9, false) /* hours */, Nothing() /* minutes */, Nothing() /* seconds */,
                                   TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(GetEventDateTime(now, dayTime, yesterday), TCivilSecond(2018, 3, 20, 21, 0, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmDateTime(now, dayTime, yesterday), Nothing());
        }

        {
            const TCivilSecond now{2018, 3, 21, 10, 0, 0};
            const TDayTime dayTime(TComponent(9, false) /* hours */, Nothing() /* minutes */, Nothing() /* seconds */,
                                   TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(GetEventDateTime(now, dayTime, yesterday), TCivilSecond(2018, 3, 20, 21, 0, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmDateTime(now, dayTime, yesterday), Nothing());
        }

        {
            const TCivilSecond now{2018, 3, 1, 22, 0, 0};
            const TDayTime dayTime(TComponent(9, false) /* hours */, Nothing() /* minutes */, Nothing() /* seconds */,
                                   TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(GetEventDateTime(now, dayTime, today), TCivilSecond(2018, 3, 1, 21, 0, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmDateTime(now, dayTime, today), Nothing());
        }

        {
            const TCivilSecond now{2018, 3, 1, 10, 0, 0};
            const TDayTime dayTime(TComponent(9, false) /* hours */, Nothing() /* minutes */, Nothing() /* seconds */,
                                   TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(GetEventDateTime(now, dayTime, today), TCivilSecond(2018, 3, 1, 21, 0, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmDateTime(now, dayTime, today), TCivilSecond(2018, 3, 1, 21, 0, 0));
        }

        {
            const TCivilSecond now{2018, 3, 1, 10, 0, 0};
            const TDayTime dayTime(TComponent(9, false) /* hours */, Nothing() /* minutes */, Nothing() /* seconds */,
                                   TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(GetEventDateTime(now, dayTime, tomorrow), TCivilSecond(2018, 3, 2, 9, 0, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmDateTime(now, dayTime, tomorrow), TCivilSecond(2018, 3, 2, 9, 0, 0));
        }

        {
            const TCivilSecond now{2018, 3, 1, 10, 0, 0};
            const TDayTime dayTime(TComponent(11, false) /* hours */, Nothing() /* minutes */, Nothing() /* seconds */,
                                   TDayTime::EPeriod::Unspecified);

            // 2018-3-2 11:00:00 is too far from now.
            UNIT_ASSERT_VALUES_EQUAL(GetEventDateTime(now, dayTime, tomorrow), TCivilSecond(2018, 3, 2, 11, 0, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmDateTime(now, dayTime, tomorrow), Nothing());
        }

        {
            const TCivilSecond now{2018, 3, 1, 12, 20, 0};
            const TDayTime dayTime(TComponent(12, false) /* hours */, TComponent(10, false) /* minutes */,
                                   Nothing() /* seconds */, TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(GetEventDateTime(now, dayTime, today), TCivilSecond(2018, 3, 2, 0, 10, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmDateTime(now, dayTime, today), Nothing());
        }

        {
            const TCivilSecond now{2018, 3, 1, 12, 20, 0};
            const TDayTime dayTime(TComponent(12, false) /* hours */, Nothing() /* minutes */, Nothing() /* seconds */,
                                   TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(GetEventDateTime(now, dayTime, today), TCivilSecond(2018, 3, 2, 0, 0, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmDateTime(now, dayTime, today), TCivilSecond(2018, 3, 2, 0, 0, 0));
        }

        {
            const TCivilSecond now{2018, 3, 1, 12, 20, 0};
            const TDayTime dayTime(TComponent(1, false) /* hours */, TComponent(10, false) /* minutes */,
                                   Nothing() /* seconds */, TDayTime::EPeriod::PM);
            UNIT_ASSERT_VALUES_EQUAL(GetEventDateTime(now, dayTime, today), TCivilSecond(2018, 3, 1, 13, 10, 0));
            UNIT_ASSERT_VALUES_EQUAL(GetAlarmDateTime(now, dayTime, today), TCivilSecond(2018, 3, 1, 13, 10, 0));
        }
    }

    Y_UNIT_TEST(WeekdaysSmoke) {
        {
            // Local date/time is 2018-02-27 12:20:30 Tue, and user asks
            // to set alarm on 7:30, for Mon and Thu.  The nearest
            // required weekday is Thu, so the correct local alarm
            // date/time is 2018-03-01 7:30:00.
            const auto alarm = GetAlarmWeekdays(
                TIME, MoscowTZ(), TDayTime{TComponent(7, false) /* hours */, TComponent(30, false) /* minutes */,
                                           Nothing() /* seconds */, TDayTime::EPeriod::Unspecified},
                TWeekdays{{TWeekday::monday, TWeekday::thursday}, true /* repeat */});
            UNIT_ASSERT_VALUES_EQUAL(alarm, TWeekdaysAlarm(MoscowTime(2018, 3, 1, 7, 30, 0),
                                                           {TWeekday::monday, TWeekday::thursday}, true /* repeat */));
        }

        {
            // Local date/time is 2018-02-27 12:20:30 Tue, and user
            // asks to set alarm on 2 hours, for Tue and Wed.  The
            // nearest trigger time is 2018-02-28, 2:00:00 Wed.
            const auto alarm =
                GetAlarmWeekdays(TIME, MoscowTZ(), TDayTime{TComponent(2, false) /* hours */, Nothing() /* minutes */,
                                                            Nothing() /* seconds */, TDayTime::EPeriod::Unspecified},
                                 TWeekdays{{TWeekday::tuesday, TWeekday::wednesday}, true /* repeat */});
            UNIT_ASSERT_VALUES_EQUAL(alarm,
                                     TWeekdaysAlarm(MoscowTime(2018, 2, 28, 2, 0, 0),
                                                    {TWeekday::monday, TWeekday::tuesday}, true /* repeat */));
        }
    }

    Y_UNIT_TEST(WeekdaysShift) {
        // Local date/time is 2018-02-27 12:20:30 Tue, and user asks
        // to set alarm on 1:20, for Mon and Thu. The nearest required
        // weekday is Thu, so the correct local alarm date/time is
        // 2018-03-01 1:20:00, and we need to subtract 3 hours because
        // of conversion to UTC. Therefore, correct UTC weekdays are
        // Wed and Sun.
        const auto alarm = GetAlarmWeekdays(
            TIME, MoscowTZ(), TDayTime{TComponent(1, false) /* hours */, TComponent(20, false) /* minutes */,
                                       Nothing() /* seconds */, TDayTime::EPeriod::Unspecified},
            TWeekdays{{TWeekday::monday, TWeekday::thursday}, true /* repeat */});
        UNIT_ASSERT_VALUES_EQUAL(alarm, TWeekdaysAlarm(MoscowTime(2018, 3, 1, 1, 20, 0),
                                                       {TWeekday::wednesday, TWeekday::sunday}, true /* repeat */));
    }

    Y_UNIT_TEST(WeekdaysAll) {
        // Local date/time is 2018-02-27 12:20:30 Tue, and user asks
        // to set alarm on 1:20, for all weekdays. The nearest trigger
        // time is 2018-02-28 1:20:00.
        const TVector<TWeekday> all{{TWeekday::monday, TWeekday::tuesday, TWeekday::wednesday, TWeekday::thursday,
                                     TWeekday::friday, TWeekday::saturday, TWeekday::sunday}};
        const auto alarm = GetAlarmWeekdays(
            TIME, MoscowTZ(), TDayTime(TComponent(1, false) /* hours */, TComponent(20, false) /* minutes
                */,
                                       Nothing() /* seconds */, TDayTime::EPeriod::Unspecified),
            TWeekdays{all, false /* repeat */});
        UNIT_ASSERT_VALUES_EQUAL(alarm, TWeekdaysAlarm(MoscowTime(2018, 2, 28, 1, 20, 0), all, false /* repeat */));
    }

    Y_UNIT_TEST(WeekdaysNone) {
        // We can't set weekday alarm when weekdays are not specified.
        const auto alarm = GetAlarmWeekdays(
            TIME, MoscowTZ(), TDayTime(TComponent(1, false) /* hours */, TComponent(20, false) /* minutes */,
                                       Nothing() /* seconds */, TDayTime::EPeriod::Unspecified),
            TWeekdays{{}, false /* repeat */});
        UNIT_ASSERT_VALUES_EQUAL(alarm, TWeekdaysAlarm{});
    }
}

Y_UNIT_TEST_SUITE(HelpersAlarmsSerialization) {
    Y_UNIT_TEST(SingleShotAlarm) {
        UNIT_ASSERT_STRINGS_EQUAL(TWeekdaysAlarm(TIME).ToICalendar(), SINGLE_SHOT_ALARM);
    }

    Y_UNIT_TEST(SingleWeekWeekdaysAlarm) {
        TWeekdaysAlarm alarm(TIME, TVector<TWeekday>{{TWeekday::monday, TWeekday::tuesday, TWeekday::wednesday}},
                             false /* Repeat */);
        UNIT_ASSERT_STRINGS_EQUAL(alarm.ToICalendar(), SINGLE_WEEK_WEEKDAYS_ALARM);
    }

    Y_UNIT_TEST(RepeatedWeekendsAlarm) {
        const auto alarm = GetAlarmWeekdays(TIME, MoscowTZ(), TDayTime(TComponent(10, false) /* hours */,
                                                                       TComponent(30, false) /* minutes */, Nothing(),
                                                                       TDayTime::EPeriod::Unspecified),
                                            TWeekdays{{TWeekday::saturday, TWeekday::sunday}, true /* repeat */});
        UNIT_ASSERT_STRINGS_EQUAL(alarm.ToICalendar(), REPEATED_WEEKENDS_ALARM);
    }
}

/*
Y_UNIT_TEST_SUITE(ApplyDate) {
    Y_UNIT_TEST(Common) {
        struct TTestItem {
            TDate Date;
            TMaybe<TCivilDay> Result;
            TStringBuf Message;
        };

        const TCivilSecond now{2018, 5, 25, 2, 23, 10}; // 25 мая 2018

        const TTestItem items[] = {
            {{Nothing(), On(4), On(24)}, Nothing(), "24 апреля"},
            {{Nothing(), On(1), On(24)}, TCivilDay{2019, 1, 24}, "24 января"},
            {{Nothing(), On(2), On(29)}, TCivilDay{2019, 3, 1}, "29 февраля"},
            {{Nothing(), Nothing(), On(24)}, Nothing(), "24 числа"},
            {{Nothing(), Nothing(), On(25)}, TCivilDay{2018, 5, 25}, "25 числа"},
            {{Nothing(), Nothing(), On(26)}, TCivilDay{2018, 5, 26}, "26 числа"},
            {{Nothing(), After(1), On(24)}, TCivilDay{2018, 6, 24}, "24 следующего месяца"},
            {{Nothing(), After(-1), On(24)}, Nothing(), "24 предыдущего месяца"},
        };

        const TGetEvent::TOptions options{.PastBadDurationDays = 62};
        for (const TTestItem& item : items) {
            TGetEvent output{now, item.Date, options};
            UNIT_ASSERT_EQUAL_C(output, item.Result.Defined(), item.Message);
            if (item.Result) {
                UNIT_ASSERT_EQUAL_C(TCivilDay(*output), *item.Result, item.Message);
            }
        }
    }
}
*/
} // namespace
