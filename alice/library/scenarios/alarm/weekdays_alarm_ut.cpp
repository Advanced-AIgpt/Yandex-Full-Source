#include "weekdays_alarm.h"

#include "helpers.h"

#include <library/cpp/timezone_conversion/civil.h>
#include <library/cpp/timezone_conversion/convert.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>

using namespace NAlice::NScenarios::NAlarm;

namespace {
constexpr TStringBuf THREE_ALARMS = TStringBuf(R"(BEGIN:VCALENDAR
BEGIN:VEVENT
DTSTART;VALUE=DATE-TIME;TZID=America/New_York:20180302T070000
DTEND;VALUE=DATE-TIME;TZID=America/New_York:20180302T070000
BEGIN:VALARM
TRIGGER:P0D
ACTION:AUDIO
END:VALARM
END:VEVENT
BEGIN:VEVENT
DTSTART;VALUE=DATE:20180303
DTEND;VALUE=DATE:20180303
BEGIN:VALARM
TRIGGER:P0D
ACTION:AUDIO
END:VALARM
END:VEVENT
BEGIN:VEVENT
DTSTART:20180304T083000Z
DTEND:20180304T083000Z
BEGIN:VALARM
TRIGGER:P0D
ACTION:AUDIO
END:VALARM
END:VEVENT
END:VCALENDAR
)");

Y_UNIT_TEST_SUITE(WeekdaysAlarmUnitTest) {
    Y_UNIT_TEST(RestSingleShot) {
        const auto tz = NDatetime::GetUtcTimeZone();
        const auto now = NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 19, 15, 0, 0}, tz);

        {
            const auto trigger = NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 19, 15, 0, 1}, tz);
            const TWeekdaysAlarm alarm{trigger};
            UNIT_ASSERT_VALUES_EQUAL(alarm.GetRest(now), alarm);
        }

        {
            const auto trigger = NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 19, 15, 0, 0}, tz);
            const TWeekdaysAlarm alarm{trigger};
            UNIT_ASSERT_VALUES_EQUAL(alarm.GetRest(now), alarm);
        }

        {
            const auto trigger = NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 19, 14, 59, 59}, tz);
            const TWeekdaysAlarm alarm{trigger};
            UNIT_ASSERT_VALUES_EQUAL(alarm.GetRest(now), Nothing());
        }

        {
            const auto trigger = NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 20, 15, 0, 0}, tz);
            const TWeekdaysAlarm alarm{trigger};
            UNIT_ASSERT_VALUES_EQUAL(alarm.GetRest(now), alarm);
        }

        {
            const auto trigger = NDatetime::Convert(NDatetime::TCivilSecond{2017, 3, 20, 15, 0, 0}, tz);
            const TWeekdaysAlarm alarm{trigger};
            UNIT_ASSERT_VALUES_EQUAL(alarm.GetRest(now), Nothing());
        }
    }

    Y_UNIT_TEST(RestWeekdaysRepeat) {
        const auto tz = NDatetime::GetUtcTimeZone();
        const auto now = NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 19, 15, 0, 0}, tz);

        {
            const TVector<NDatetime::TWeekday> days = {{NDatetime::TWeekday::monday, NDatetime::TWeekday::wednesday}};
            const auto begin = NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 19, 15, 1, 0}, tz);
            const TWeekdaysAlarm alarm(begin, days, true /* repeat */);

            UNIT_ASSERT(!alarm.IsEmpty());
            UNIT_ASSERT_VALUES_EQUAL(alarm.GetRest(now), alarm);
        }

        {
            const TVector<NDatetime::TWeekday> days = {{NDatetime::TWeekday::monday, NDatetime::TWeekday::wednesday}};
            const auto begin = NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 19, 14, 59, 0}, tz);
            TWeekdaysAlarm alarm(begin, days, true /* repeat */);

            UNIT_ASSERT(!alarm.IsEmpty());
            UNIT_ASSERT_VALUES_EQUAL(alarm.GetRest(now), alarm);
        }

        {
            const TVector<NDatetime::TWeekday> days;
            const auto begin = NDatetime::Convert(NDatetime::TCivilSecond{2017, 3, 19, 15, 1, 0}, tz);
            const TWeekdaysAlarm alarm(begin, days, true /* repeat */);

            UNIT_ASSERT(alarm.IsEmpty());

            // Even when alarm begin is in the past, no need to modify it in GetRest().
            UNIT_ASSERT_VALUES_EQUAL(alarm.GetRest(now), Nothing());
        }
    }

    Y_UNIT_TEST(RestWeekdaysSingle) {
        const auto tz = NDatetime::GetUtcTimeZone();
        const auto now = NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 19, 15, 0, 0}, tz);

        {
            const TVector<NDatetime::TWeekday> days = {{NDatetime::TWeekday::monday, NDatetime::TWeekday::wednesday}};
            const auto begin = NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 19, 15, 1, 0}, tz);
            const TWeekdaysAlarm alarm(begin, days, false /* repeat */);

            UNIT_ASSERT(!alarm.IsEmpty());
            UNIT_ASSERT_VALUES_EQUAL(alarm.GetRest(now), alarm);
        }

        {
            const TVector<NDatetime::TWeekday> days = {{NDatetime::TWeekday::monday, NDatetime::TWeekday::wednesday}};
            const auto begin = NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 19, 14, 59, 0}, tz);
            const TWeekdaysAlarm alarm(begin, days, false /* repeat */);

            UNIT_ASSERT(!alarm.IsEmpty());

            const TWeekdaysAlarm expected(NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 21, 14, 59, 0}, tz),
                                          {NDatetime::TWeekday::wednesday}, false /* repeat */);
            UNIT_ASSERT_VALUES_EQUAL(alarm.GetRest(now), expected);
        }

        {
            const TVector<NDatetime::TWeekday> days = {{NDatetime::TWeekday::monday, NDatetime::TWeekday::wednesday}};
            const auto begin = NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 19, 14, 59, 0}, tz);
            const TWeekdaysAlarm alarm(begin, days, false /* repeat */);

            UNIT_ASSERT(!alarm.IsEmpty());

            UNIT_ASSERT_VALUES_EQUAL(
                alarm.GetRest(NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 22, 14, 59, 0}, tz) /* now */),
                Nothing());
        }
    }

    Y_UNIT_TEST(IsSubsetIrregularIrregular) {
        {
            const TWeekdaysAlarm a{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 10, 30, 0})};
            const TWeekdaysAlarm b{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 10, 30, 0})};

            const TWeekdaysAlarm c{FromUTC(NDatetime::TCivilSecond{2018, 3, 2, 10, 30, 0})};
            const TWeekdaysAlarm d{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 10, 30, 1})};

            UNIT_ASSERT(a.IsSubsetOf(b));
            UNIT_ASSERT(b.IsSubsetOf(a));

            UNIT_ASSERT(!a.IsSubsetOf(c));
            UNIT_ASSERT(!c.IsSubsetOf(a));

            UNIT_ASSERT(!a.IsSubsetOf(d));
            UNIT_ASSERT(!d.IsSubsetOf(a));
        }

        {
            // 2018-3-1 9:30 is thursday.
            const TWeekdaysAlarm a{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 9, 30, 0})};
            UNIT_ASSERT(a.IsSubsetOf(a));

            const TWeekdaysAlarm b{
                FromUTC(NDatetime::TCivilSecond{2018, 2, 28, 9, 30, 0}),
                {NDatetime::TWeekday::thursday, NDatetime::TWeekday::friday, NDatetime::TWeekday::saturday},
                false /* repeat */};
            UNIT_ASSERT(b.IsSubsetOf(b));

            UNIT_ASSERT(a.IsSubsetOf(b));
            UNIT_ASSERT(!b.IsSubsetOf(a));

            const TWeekdaysAlarm c{FromUTC(NDatetime::TCivilSecond{2018, 2, 28, 9, 30, 0}),
                                   {NDatetime::TWeekday::wednesday, NDatetime::TWeekday::friday},
                                   false /* repeat */};
            UNIT_ASSERT(c.IsSubsetOf(c));

            UNIT_ASSERT(!a.IsSubsetOf(c));
            UNIT_ASSERT(!c.IsSubsetOf(a));

            const TWeekdaysAlarm d{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 9, 30, 0}),
                                   {NDatetime::TWeekday::thursday},
                                   false /* repeat */};
            UNIT_ASSERT(d.IsSubsetOf(d));

            UNIT_ASSERT(a.IsSubsetOf(d));
            UNIT_ASSERT(d.IsSubsetOf(a));

            UNIT_ASSERT(d.IsSubsetOf(b));
            UNIT_ASSERT(!b.IsSubsetOf(d));

            UNIT_ASSERT(!c.IsSubsetOf(b));
            UNIT_ASSERT(!b.IsSubsetOf(c));

            const TWeekdaysAlarm e{
                FromUTC(NDatetime::TCivilSecond{2018, 2, 28, 9, 45, 0}),
                {NDatetime::TWeekday::thursday, NDatetime::TWeekday::friday, NDatetime::TWeekday::saturday},
                false /* repeat */};
            UNIT_ASSERT(e.IsSubsetOf(e));

            UNIT_ASSERT(!a.IsSubsetOf(e));
            UNIT_ASSERT(!e.IsSubsetOf(a));
        }
    }

    Y_UNIT_TEST(IsSubsetIrregularRegular) {
        // 2018-3-1 9:30 is thursday.
        const TWeekdaysAlarm a{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 9, 30, 0})};
        UNIT_ASSERT(a.IsSubsetOf(a));

        const TWeekdaysAlarm b{FromUTC(NDatetime::TCivilSecond{2018, 3, 19, 9, 30, 0}),
                               {NDatetime::TWeekday::monday, NDatetime::TWeekday::tuesday,
                                NDatetime::TWeekday::wednesday, NDatetime::TWeekday::thursday,
                                NDatetime::TWeekday::friday},
                               true /* repeat */};
        UNIT_ASSERT(b.IsSubsetOf(b));

        UNIT_ASSERT(a.IsSubsetOf(b));
        UNIT_ASSERT(!b.IsSubsetOf(a));

        const TWeekdaysAlarm c{FromUTC(NDatetime::TCivilSecond{2018, 3, 19, 9, 30, 0}),
                               {NDatetime::TWeekday::saturday, NDatetime::TWeekday::sunday},
                               true /* repeat */};
        UNIT_ASSERT(c.IsSubsetOf(c));

        UNIT_ASSERT(!a.IsSubsetOf(c));
        UNIT_ASSERT(!c.IsSubsetOf(a));

        const TWeekdaysAlarm d{FromUTC(NDatetime::TCivilSecond{2018, 3, 19, 9, 30, 0}),
                               {NDatetime::TWeekday::thursday},
                               true /* repeat */};
        UNIT_ASSERT(d.IsSubsetOf(d));

        UNIT_ASSERT(a.IsSubsetOf(d));
        UNIT_ASSERT(!d.IsSubsetOf(a));

        const TWeekdaysAlarm e{FromUTC(NDatetime::TCivilSecond{2018, 3, 19, 9, 45, 0}),
                               {NDatetime::TWeekday::thursday},
                               true /* repeat */};
        UNIT_ASSERT(e.IsSubsetOf(e));

        UNIT_ASSERT(!a.IsSubsetOf(e));
        UNIT_ASSERT(!e.IsSubsetOf(a));
    }

    Y_UNIT_TEST(IsSubsetRegularRegular) {
        // 2018-3-1 9:30 is thursday.
        const TWeekdaysAlarm a{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 9, 30, 0}),
                               {NDatetime::TWeekday::thursday, NDatetime::TWeekday::friday},
                               true /* repeat */};
        UNIT_ASSERT(a.IsSubsetOf(a));

        // 2017-10-4 9:30 is wednesday.
        const TWeekdaysAlarm b{
            FromUTC(NDatetime::TCivilSecond{2017, 10, 4, 9, 30, 0}),
            {NDatetime::TWeekday::wednesday, NDatetime::TWeekday::thursday, NDatetime::TWeekday::friday},
            true /* repeat */};
        UNIT_ASSERT(b.IsSubsetOf(b));

        UNIT_ASSERT(a.IsSubsetOf(b));
        UNIT_ASSERT(!b.IsSubsetOf(a));

        // 2017-10-6 9:30 is friday.
        const TWeekdaysAlarm c{
            FromUTC(NDatetime::TCivilSecond{2017, 10, 6, 9, 30, 0}), {NDatetime::TWeekday::friday}, true /* repeat */};
        UNIT_ASSERT(c.IsSubsetOf(c));

        UNIT_ASSERT(c.IsSubsetOf(a));
        UNIT_ASSERT(!a.IsSubsetOf(c));
    }

    Y_UNIT_TEST(TriggersOnDayTimeSmoke) {
        // First alarm time is 2018-3-1 23:30 MSK, repeat on thursday
        // and friday.
        const TWeekdaysAlarm a{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 20, 30, 0}),
                               {NDatetime::TWeekday::thursday, NDatetime::TWeekday::friday},
                               true /* repeat */};

        const NDatetime::TCivilSecond now{2017, 4, 2, 10, 30, 0};
        const auto tz = NDatetime::GetTimeZone("Europe/Moscow");

        UNIT_ASSERT(a.TriggersOnDayTime(TDayTime(11, 30, 0, TDayTime::EPeriod::Unspecified), now, tz));
        UNIT_ASSERT(!a.TriggersOnDayTime(TDayTime(11, 30, 0, TDayTime::EPeriod::AM), now, tz));
        UNIT_ASSERT(a.TriggersOnDayTime(TDayTime(11, 30, 0, TDayTime::EPeriod::PM), now, tz));

        UNIT_ASSERT(a.TriggersOnDayTime(TDayTime(23, 30, 0, TDayTime::EPeriod::Unspecified), now, tz));

        UNIT_ASSERT(!a.TriggersOnDayTime(TDayTime(After(12) /* hours */, Nothing() /* minutes */,
                                                  Nothing() /* seconds */, TDayTime::EPeriod::Unspecified),
                                         now, tz));

        UNIT_ASSERT(a.TriggersOnDayTime(TDayTime(After(13) /* hours */, Nothing() /* minutes */,
                                                 Nothing() /* seconds */, TDayTime::EPeriod::Unspecified),
                                        now, tz));

        UNIT_ASSERT(!a.TriggersOnDayTime(TDayTime(After(14) /* hours */, Nothing() /* minutes */,
                                                  Nothing() /* seconds */, TDayTime::EPeriod::Unspecified),
                                         now, tz));
    }

    Y_UNIT_TEST(TriggersOnlyOnDateSmoke) {
        const NDatetime::TCivilSecond now{2018, 2, 27, 10, 30, 0};
        const auto tz = NDatetime::GetTimeZone("Europe/Moscow");

        const auto triggers = [&now, &tz](const TWeekdaysAlarm& alarm, const TDate& date) {
            return alarm.TriggersOnlyOnDate(date, now, tz);
        };

        // Alarm time is 2018-3-1 23:30 MSK, thursday.
        const TWeekdaysAlarm a{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 20, 30, 0})};

        UNIT_ASSERT(triggers(a, TDate(Nothing() /* years */, On(3) /* months */, On(1) /* days */)));
        UNIT_ASSERT(!triggers(a, TDate(Nothing() /* years */, On(3) /* months */, On(2) /* days */)));
        UNIT_ASSERT(triggers(a, TDate(Nothing() /* years */, Nothing() /* months */, After(2) /* days */)));

        // First alarm time is 2018-3-1 23:30 MSK, thursday, repeat on thursday and friday.
        const TWeekdaysAlarm b{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 20, 30, 0}),
                               {NDatetime::TWeekday::thursday, NDatetime::TWeekday::friday},
                               true /* repeat */};

        UNIT_ASSERT(!triggers(b, TDate(Nothing() /* years */, On(3) /* months */, On(1) /* days */)));
    }

    Y_UNIT_TEST(TriggersOnDateSmoke) {
        const NDatetime::TCivilSecond now{2018, 2, 27, 10, 30, 0};
        const auto tz = NDatetime::GetTimeZone("Europe/Moscow");

        const auto triggers = [&now, &tz](const TWeekdaysAlarm& alarm, const TDate& date) {
            return alarm.TriggersOnDate(date, now, tz);
        };

        // Alarm time is 2018-3-1 23:30 MSK, thursday.
        const TWeekdaysAlarm a{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 20, 30, 0})};

        UNIT_ASSERT(triggers(a, TDate(Nothing() /* years */, On(3) /* months */, On(1) /* days */)));
        UNIT_ASSERT(!triggers(a, TDate(Nothing() /* years */, On(3) /* months */, On(2) /* days */)));
        UNIT_ASSERT(triggers(a, TDate(Nothing() /* years */, Nothing() /* months */, After(2) /* days */)));

        // First alarm time is 2018-3-1 23:30 MSK, thursday, repeat on thursday and friday.
        const TWeekdaysAlarm b{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 20, 30, 0}),
                               {NDatetime::TWeekday::thursday, NDatetime::TWeekday::friday},
                               true /* repeat */};

        UNIT_ASSERT(triggers(b, TDate(Nothing() /* years */, On(3) /* months */, On(1) /* days */)));
        UNIT_ASSERT(triggers(b, TDate(Nothing() /* years */, On(3) /* months */, On(2) /* days */)));

        UNIT_ASSERT(!triggers(b, TDate(Nothing() /* years */, On(3) /* months */, On(3) /* days */)));
        UNIT_ASSERT(!triggers(b, TDate(Nothing() /* years */, On(3) /* months */, On(7) /* days */)));

        UNIT_ASSERT(triggers(b, TDate(Nothing() /* years */, On(3) /* months */, On(8) /* days */)));
        UNIT_ASSERT(triggers(b, TDate(Nothing() /* years */, On(3) /* months */, On(9) /* days */)));

        UNIT_ASSERT(!triggers(b, TDate(Nothing() /* years */, Nothing() /* months */, After(1) /* days */)));
        UNIT_ASSERT(triggers(b, TDate(Nothing() /* years */, Nothing() /* months */, After(2) /* days */)));
        UNIT_ASSERT(triggers(b, TDate(Nothing() /* years */, Nothing() /* months */, After(3) /* days */)));
        UNIT_ASSERT(!triggers(b, TDate(Nothing() /* years */, Nothing() /* months */, After(4) /* days */)));
    }

    Y_UNIT_TEST(TriggersOnSameWeekdaysSmoke) {
        using TWeekday = NDatetime::TWeekday;

        const auto tz = NDatetime::GetTimeZone("Europe/Moscow");

        const auto triggers = [&tz](const TWeekdaysAlarm& alarm, const TWeekdays& weekdays) {
            return alarm.TriggersOnSameWeekdays(weekdays, tz);
        };

        // Alarm time is 2018-3-1 23:30 MSK, thursday.
        const TWeekdaysAlarm a{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 20, 30, 0})};

        UNIT_ASSERT(triggers(a, TWeekdays({TWeekday::thursday}, false /* repeat */)));
        UNIT_ASSERT(!triggers(a, TWeekdays({TWeekday::thursday}, true /* repeat */)));
        UNIT_ASSERT(!triggers(a, TWeekdays({TWeekday::friday}, false /* repeat */)));
        UNIT_ASSERT(!triggers(a, TWeekdays({TWeekday::friday}, true /* repeat */)));

        // First alarm time is 2018-3-1 23:30 MSK, thursday, repeat on thursday and friday.
        const TWeekdaysAlarm b{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 20, 30, 0}),
                               {NDatetime::TWeekday::thursday, NDatetime::TWeekday::friday},
                               true /* repeat */};

        UNIT_ASSERT(triggers(b, TWeekdays({TWeekday::friday, TWeekday::thursday}, true /* repeat */)));
        UNIT_ASSERT(triggers(b, TWeekdays({TWeekday::thursday, TWeekday::friday}, true /* repeat */)));
        UNIT_ASSERT(!triggers(b, TWeekdays({TWeekday::thursday, TWeekday::friday}, false /* repeat */)));
        UNIT_ASSERT(!triggers(b, TWeekdays({TWeekday::thursday}, true /* repeat */)));
        UNIT_ASSERT(!triggers(b, TWeekdays({TWeekday::thursday}, false /* repeat */)));
    }

    Y_UNIT_TEST(TriggersOnWeekdaysSmoke) {
        using TWeekday = NDatetime::TWeekday;

        const auto tz = NDatetime::GetTimeZone("Europe/Moscow");

        const auto triggers = [&tz](const TWeekdaysAlarm& alarm, const TWeekdays& weekdays) {
            return alarm.TriggersOnWeekdays(weekdays, tz);
        };

        // Alarm time is 2018-3-1 23:30 MSK, thursday.
        const TWeekdaysAlarm a{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 20, 30, 0})};

        UNIT_ASSERT(triggers(a, TWeekdays({TWeekday::thursday}, false /* repeat */)));
        UNIT_ASSERT(triggers(a, TWeekdays({TWeekday::thursday}, true /* repeat */)));
        UNIT_ASSERT(!triggers(a, TWeekdays({TWeekday::friday}, false /* repeat */)));
        UNIT_ASSERT(!triggers(a, TWeekdays({TWeekday::friday}, true /* repeat */)));

        // First alarm time is 2018-3-1 23:30 MSK, thursday, repeat on thursday and friday.
        const TWeekdaysAlarm b{FromUTC(NDatetime::TCivilSecond{2018, 3, 1, 20, 30, 0}),
                               {NDatetime::TWeekday::thursday, NDatetime::TWeekday::friday},
                               true /* repeat */};

        UNIT_ASSERT(triggers(b, TWeekdays({TWeekday::friday, TWeekday::thursday}, true /* repeat */)));
        UNIT_ASSERT(triggers(b, TWeekdays({TWeekday::thursday, TWeekday::friday}, true /* repeat */)));
        UNIT_ASSERT(triggers(b, TWeekdays({TWeekday::thursday, TWeekday::friday}, false /* repeat */)));
        UNIT_ASSERT(triggers(b, TWeekdays({TWeekday::thursday}, true /* repeat */)));
        UNIT_ASSERT(triggers(b, TWeekdays({TWeekday::thursday}, false /* repeat */)));
        UNIT_ASSERT(!triggers(b, TWeekdays({TWeekday::monday, TWeekday::thursday}, false /* repeat */)));
    }

    Y_UNIT_TEST(FromICalendarSmoke) {
        const auto msk = NDatetime::GetTimeZone("Europe/Moscow");
        const auto ny = NDatetime::GetTimeZone("America/New_York");
        const auto utc = NDatetime::GetUtcTimeZone();

        TVector<TWeekdaysAlarm> expected;
        expected.emplace_back(TWeekdaysAlarm(NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 2, 7, 0, 0}, ny)));
        expected.emplace_back(TWeekdaysAlarm(NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 3, 0, 0, 0}, msk)));
        expected.emplace_back(TWeekdaysAlarm(NDatetime::Convert(NDatetime::TCivilSecond{2018, 3, 4, 8, 30, 0}, utc)));

        const auto actual = TWeekdaysAlarm::FromICalendar(msk, THREE_ALARMS);
        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }
}
} // namespace
