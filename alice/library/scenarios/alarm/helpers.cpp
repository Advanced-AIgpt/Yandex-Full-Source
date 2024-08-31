#include "helpers.h"

#include "date_time.h"

#include <library/cpp/timezone_conversion/convert.h>

#include <util/generic/array_ref.h>
#include <util/string/builder.h>
#include <util/string/join.h>

namespace NAlice::NScenarios::NAlarm {

namespace {

constexpr int NUM_WEEKDAYS = 7;

} // namespace

TInstant ToInstant(const NDatetime::TCivilSecond& now, const NDatetime::TTimeZone& tz) {
    return NDatetime::Convert(now, tz);
}

TInstant FromUTC(const NDatetime::TCivilSecond& now) {
    return ToInstant(now, NDatetime::GetUtcTimeZone());
}

NDatetime::TCivilSecond ToUTC(const TInstant& now) {
    return NDatetime::Convert(now, NDatetime::GetUtcTimeZone());
}

NDatetime::TCivilSecond ToUTC(const NDatetime::TCivilSecond& now, const NDatetime::TTimeZone& tz) {
    return NDatetime::Convert(ToInstant(now, tz), NDatetime::GetUtcTimeZone());
}

NDatetime::TWeekday GetWeekday(const NDatetime::TCivilSecond& now) {
    return NDatetime::GetWeekday(NDatetime::TCivilDay{now});
}

bool SameDayTime(const NDatetime::TCivilSecond& a, const NDatetime::TCivilSecond& b) {
    return a.hour() == b.hour() && a.minute() == b.minute() && a.second() == b.second();
}

bool SameDayTime(const TInstant& a, const TInstant& b) {
    return SameDayTime(ToUTC(a), ToUTC(b));
}

bool SameDate(const NDatetime::TCivilSecond& a, const NDatetime::TCivilSecond& b) {
    return a.year() == b.year() && a.month() == b.month() && a.day() == b.day();
}

bool SameDate(const TInstant& a, const TInstant& b) {
    return SameDate(ToUTC(a), ToUTC(b));
}

bool IsMidnight(const NDatetime::TCivilSecond& a) {
    return a.hour() == 0 && a.minute() == 0 && a.second() == 0;
}

bool IsCivilNight(const NDatetime::TCivilSecond& a) {
    return a.hour() < 4;
}

NDatetime::TCivilSecond GetEventTime(const NDatetime::TCivilSecond& now, const TDayTime& dayTime) {
    NDatetime::TCivilSecond alarm = dayTime.Apply(now);

    if (!dayTime.IsRelative() && alarm <= now) {
        int diffHours = 24;
        int times = 1;
        if (dayTime.AmbiguousHours()) {
            diffHours = 12;
            times = 2;
        }

        for (int i = 0; i < times && alarm <= now; ++i)
            alarm = NDatetime::AddHours(alarm, diffHours);

        Y_ASSERT(alarm > now);
    }

    return alarm;
}

NDatetime::TCivilSecond GetEventTime(const NDatetime::TCivilSecond& now, NDatetime::TCivilSecond event,
                                     const TDayTime& dayTime) {
    event = dayTime.Apply(event);

    if (event <= now && SameDate(event, now) && IsMidnight(event))
        event = NDatetime::AddHours(event, 24);
    else if (event <= now && dayTime.AmbiguousHours())
        event = NDatetime::AddHours(event, 12);

    return event;
}

TMaybe<NDatetime::TCivilSecond> GetEventDateTime(const NDatetime::TCivilSecond& now, const TDayTime& dayTime,
                                                 const TMaybe<TDate>& date) {
    if (date)
        return GetEventDateTime(now, dayTime, *date);
    return GetEventTime(now, dayTime);
}

TMaybe<NDatetime::TCivilSecond> GetEventDateTime(const NDatetime::TCivilSecond& now, const TDayTime& dayTime,
                                                 const TDate& date) {
    if (!date.HasExactDay())
        return Nothing();

    // For civil nights (time between 12:00 am and 4:00 am) we need to
    // drop "tomorrow" specifier.
    if (date.IsTomorrow() && IsCivilNight(now))
        return GetEventTime(now, dayTime);

    return GetEventTime(now, date.Apply(now), dayTime);
}

NDatetime::TCivilSecond GetAlarmTime(const NDatetime::TCivilSecond& now, const TDayTime& dayTime) {
    NDatetime::TCivilSecond alarm = dayTime.Apply(now);

    if (!dayTime.IsRelative() && alarm <= now) {
        alarm = NDatetime::AddHours(alarm, 24);
        Y_ASSERT(alarm > now);
    }

    return alarm;
}

TWeekdaysAlarm GetAlarmTime(const TInstant& now, const NDatetime::TTimeZone& tz, const TDayTime& dayTime) {
    const NDatetime::TCivilSecond alarm = GetAlarmTime(NDatetime::Convert(now, tz), dayTime);
    return TWeekdaysAlarm(ToInstant(alarm, tz));
}

TMaybe<NDatetime::TCivilSecond> GetAlarmDateTime(const NDatetime::TCivilSecond& now, const TDayTime& dayTime,
                                                 const TDate& date) {
    if (!date.HasExactDay())
        return Nothing();

    if (date.IsTomorrow() && IsCivilNight(now))
        return GetAlarmTime(now, dayTime);

    NDatetime::TCivilSecond trigger = date.Apply(now);
    trigger = dayTime.Apply(trigger);

    if (trigger <= now && SameDate(trigger, now) && IsMidnight(trigger))
        trigger = NDatetime::AddHours(trigger, 24);

    // В 16:00 "Поставь будильник сегодня в 5" будет ставить будильник на 17:00
    if (trigger <= now &&
        SameDate(trigger, now) &&
        dayTime.Period == TDayTime::EPeriod::Unspecified &&
        (trigger.hour() <= 11 || trigger.hour() == 12 && trigger.minute() == 0 && trigger.second() == 0)
    ) {
        trigger = NDatetime::AddHours(trigger, 12);
    }

    if (trigger <= now || trigger >= NDatetime::AddDays(now, 1))
        return Nothing();

    return trigger;
}

TMaybe<NDatetime::TCivilSecond> GetAlarmDateTime(const NDatetime::TCivilSecond& now, const TDayTime& dayTime,
                                                 const TMaybe<TDate>& date) {
    if (date)
        return GetAlarmDateTime(now, dayTime, *date);

    return GetAlarmTime(now, dayTime);
}

TMaybe<TWeekdaysAlarm> GetAlarmDateTime(const TInstant& now, const NDatetime::TTimeZone& tz, const TDayTime& dayTime,
                                        const TDate& date) {
    const TMaybe<NDatetime::TCivilSecond> alarm = GetAlarmDateTime(NDatetime::Convert(now, tz), dayTime, date);

    if (!alarm)
        return Nothing();

    return TWeekdaysAlarm(ToInstant(*alarm, tz));
}

TWeekdaysAlarm GetAlarmWeekdays(const NDatetime::TCivilSecond& now, const NDatetime::TTimeZone& tz,
                                const TDayTime& dayTime, const TWeekdays& weekdays) {
    TWeekdaysAlarm result;

    if (weekdays.Empty())
        return result;

    result.Weekdays = TWeekdays{};
    result.Weekdays->Repeat = weekdays.Repeat;

    NDatetime::TCivilSecond curr = dayTime.Apply(now);

    while (curr <= now || !weekdays.Contains(GetWeekday(curr)))
        curr = NDatetime::AddDays(curr, 1);

    result.Begin = ToInstant(curr, tz);

    auto& days = result.Weekdays->Days;
    for (int offset = 0; offset < NUM_WEEKDAYS; ++offset, curr = NDatetime::AddDays(curr, 1)) {
        if (weekdays.Contains(GetWeekday(curr)))
            days.push_back(GetWeekday(ToUTC(curr, tz)));
    }

    std::sort(days.begin(), days.end());

    return result;
}

TWeekdaysAlarm GetAlarmWeekdays(const TInstant& now, const NDatetime::TTimeZone& tz, const TDayTime& dayTime,
                                const TWeekdays& weekdays) {
    return GetAlarmWeekdays(NDatetime::Convert(now, tz), tz, dayTime, weekdays);
}

NSc::TValue TimeToValue(const NDatetime::TCivilSecond& time) {
    auto extract = [](int value, bool keepZero) -> TMaybe<TDayTime::TComponent> {
        if (!keepZero && value == 0)
            return Nothing();
        return TDayTime::TComponent(value, false /* relative */);
    };

    const TDayTime dayTime(extract(time.hour(), true /* keepZero */), extract(time.minute(), false /* keepZero */),
                           extract(time.second(), false /* keepZero */), TDayTime::EPeriod::Unspecified);
    return dayTime.ToValue();
}

NSc::TValue TimeToValue(const TInstant& now, const NDatetime::TTimeZone& tz) {
    return TimeToValue(NDatetime::Convert(now, tz));
}

NSc::TValue DateToValue(const NDatetime::TCivilSecond& now, const TDate& date) {
    if (date.IsToday(now))
        return TDate::MakeToday().ToValue();
    if (date.IsTomorrow(now))
        return TDate::MakeTomorrow().ToValue();
    return date.ToValue();
}

NSc::TValue DateToValue(const NDatetime::TCivilSecond& now, const NDatetime::TCivilSecond& date) {
    return DateToValue(now, TDate(date));
}

bool MatchesExactlyDayTimeAndDate(
    const TWeekdaysAlarm& alarm,
    const TMaybe<TDayTime>& dayTime,
    const TMaybe<TDate>& date,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz
) {
    return !alarm.IsRegular() && alarm.TriggersOnDayTime(dayTime, now, tz) &&
           alarm.TriggersOnlyOnDate(date, now, tz);
}

bool MatchesApproximatelyDayTimeAndDate(
    const TWeekdaysAlarm& alarm,
    const TMaybe<TDayTime>& dayTime,
    const TMaybe<TDate>& date,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz
) {
    return alarm.TriggersOnDayTime(dayTime, now, tz) && alarm.TriggersOnDate(date, now, tz);
}

bool MatchesExactlyDayTimeAndWeekdays(
    const TWeekdaysAlarm& alarm,
    const TMaybe<TDayTime>& dayTime,
    const TWeekdays& weekdays,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz
) {
    return alarm.IsRegular() == weekdays.Repeat && alarm.TriggersOnDayTime(dayTime, now, tz) &&
           alarm.TriggersOnSameWeekdays(weekdays, tz);
}

bool MatchesApproximatelyDayTimeAndWeekdays(
    const TWeekdaysAlarm& alarm,
    const TMaybe<TDayTime>& dayTime,
    const TWeekdays& weekdays,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz
) {
    return alarm.TriggersOnDayTime(dayTime, now, tz) && alarm.TriggersOnWeekdays(weekdays, tz);
}

bool MatchesApproximatelyDayTime(
    const TWeekdaysAlarm& alarm,
    const TDayTime& dayTime,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz
) {
    return alarm.TriggersOnDayTime(dayTime, now, tz);
}

bool IsFirstBeforeSecond(const TDayTime& time1, const TDayTime& time2) {
    int hours1 = time1.Hours->Value;
    int minutes1 = time1.Minutes->Value;
    int seconds1 = time1.Seconds->Value;
    int hours2 = time2.Hours->Value;
    int minutes2 = time2.Minutes->Value;
    int seconds2 = time2.Seconds->Value;

    return hours2 > hours1 || hours2 == hours1 && minutes2 >= minutes1 ||
           hours2 == hours1 && minutes2 == minutes1 && seconds2 >= seconds1;
}

bool GetDistInTime(const TDayTime& time1, const TDayTime& time2, TDayTime& answer) {
    if (!IsFirstBeforeSecond(time1, time2)) {
        return false;
    }

    int hours1 = time1.Hours->Value;
    int minutes1 = time1.Minutes->Value;
    int seconds1 = time1.Seconds->Value;
    int hours2 = time2.Hours->Value;
    int minutes2 = time2.Minutes->Value;
    int seconds2 = time2.Seconds->Value;

    if (seconds2 < seconds1) {
        seconds2 += 60;
        --minutes2;
    }

    if (minutes2 < minutes1) {
        minutes2 += 60;
        --hours2;
    }
    answer = TDayTime(hours2 - hours1, minutes2 - minutes1, seconds2 - seconds1, TDayTime::EPeriod::Unspecified);
    return true;
}

} // namespace NAlice::NScenarios::NAlarm
