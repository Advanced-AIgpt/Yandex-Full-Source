#include "weekdays_alarm.h"

#include "helpers.h"

#include <alice/library/calendar_parser/iso8601.h>

#include <library/cpp/timezone_conversion/convert.h>

#include <util/generic/algorithm.h>
#include <util/stream/output.h>
#include <util/system/yassert.h>

namespace NAlice::NScenarios::NAlarm {

namespace {

constexpr int NUM_WEEKDAYS = 7;

template <typename T, typename... TArgs>
T& Emplace(TVector<T>& values, TArgs&&... args) {
    values.emplace_back(std::forward<TArgs>(args)...);
    return values.back();
}

} // namespace

// TWeekdaysAlarm --------------------------------------------------------------
// static
TMaybe<TWeekdaysAlarm> TWeekdaysAlarm::FromEvent(const NDatetime::TTimeZone& tz,
                                                 const NCalendarParser::TEvent& event) {
    if (!event.Start) {
        return Nothing();
    }

    const auto& start = *event.Start;

    TWeekdaysAlarm alarm;

    if (start.TimeZone)
        alarm.Begin = NDatetime::Convert(start.Time, *start.TimeZone);
    else
        alarm.Begin = NDatetime::Convert(start.Time, tz);

    if (!event.Recur)
        return alarm;

    const auto& recur = *event.Recur;

    auto& weekdays = alarm.Weekdays.ConstructInPlace(recur.Weekdays, true /* repeat */);

    if (recur.Until) {
        const auto begin = ToUTC(alarm.Begin);
        const auto end = ToUTC(*recur.Until);

        if (NDatetime::AddDays(begin, 7) != end) {
            return Nothing();
        }

        weekdays.Repeat = false;
    }

    return alarm;
}

void TWeekdaysAlarm::Append(NCalendarParser::TCalendar& calendar, TVector<TString>& lines) const {
    auto& event = Emplace(calendar.Events);
    event.Start = NCalendarParser::TStart(ToUTC(Begin), NDatetime::GetUtcTimeZone());
    event.End = NCalendarParser::TEnd(ToUTC(Begin), NDatetime::GetUtcTimeZone());

    if (Weekdays) {
        NCalendarParser::TWeeklyRecur recur;
        recur.Weekdays = Weekdays->Days;

        if (!Weekdays->Repeat) {
            const auto End = NDatetime::AddDays(ToUTC(Begin), 7);
            recur.Until = FromUTC(End);
        }

        event.Recur = recur;
    }

    const auto& dateTime = Emplace(lines, NCalendarParser::TISO8601SerDes::Ser(Begin));

    auto& alarm = Emplace(event.Alarms);
    if (Weekdays) {
        alarm.Body.emplace_back("TRIGGER", "P0D");
    } else {
        alarm.Body.emplace_back("TRIGGER", dateTime,
                                TVector<NCalendarParser::TParam>{{"VALUE", TVector<TStringBuf>{"DATE-TIME"}}});
    }
    alarm.Body.emplace_back("ACTION", "AUDIO");
}

TString TWeekdaysAlarm::ToICalendar() const {
    return TWeekdaysAlarm::ToICalendar({*this});
}

// static
TVector<TWeekdaysAlarm> TWeekdaysAlarm::FromICalendar(const NDatetime::TTimeZone& tz, TStringBuf data) {
    TVector<TWeekdaysAlarm> alarms;

    if (data.empty())
        return alarms;

    NCalendarParser::TCalendar calendar;
    TVector<TString> lines;
    NCalendarParser::TParser().Parse(data, calendar, lines);

    for (const auto& event : calendar.Events) {
        if (const auto alarm = TWeekdaysAlarm::FromEvent(tz, event))
            alarms.push_back(*alarm);
    }

    return alarms;
}

// static
TString TWeekdaysAlarm::ToICalendar(const TVector<TWeekdaysAlarm>& alarms) {
    auto calendar = NCalendarParser::TCalendar::MakeDefault();
    TVector<TString> lines;
    for (const auto& alarm : alarms)
        alarm.Append(calendar, lines);
    return calendar.Serialize();
}

// static
NSc::TValue TWeekdaysAlarm::ToICalendarPayload(const TVector<TWeekdaysAlarm>& alarms, const bool listeningIsPossible) {
    NSc::TValue payload;
    payload["state"] = ToICalendar(alarms);
    if (listeningIsPossible) {
        payload["listening_is_possible"].SetBool(true);
    }
    return payload;
}

TVector<NDatetime::TWeekday> TWeekdaysAlarm::GetTriggerWeekdays(const NDatetime::TTimeZone& tz) const {
    TVector<NDatetime::TWeekday> weekdays;

    if (IsRegular()) {
        const TMaybe<TWeekdays> ws = GetLocalWeekdays(tz);
        if (ws)
            weekdays = ws->Days;
        else
            Y_ASSERT(false);
    } else {
        const auto triggers = GetNonRegularAlarmTriggers();
        for (const auto& trigger : triggers)
            weekdays.push_back(GetWeekday(NDatetime::Convert(trigger, tz)));
    }

    SortUnique(weekdays);
    return weekdays;
}

TMaybe<TWeekdays> TWeekdaysAlarm::GetLocalWeekdays(const NDatetime::TTimeZone& tz) const {
    if (!Weekdays)
        return Nothing();

    auto utc = NDatetime::Convert(Begin, NDatetime::GetUtcTimeZone());
    auto loc = NDatetime::Convert(Begin, tz);

    TVector<NDatetime::TWeekday> locDays;
    for (int i = 0; i < NUM_WEEKDAYS; ++i, utc = NDatetime::AddDays(utc, 1), loc = NDatetime::AddDays(loc, 1)) {
        if (Weekdays->Contains(GetWeekday(utc)))
            locDays.emplace_back(GetWeekday(loc));
    }

    // For single-week alarms it's more user-friendly to keep weekdays
    // ordered by date.
    if (Weekdays->Repeat)
        Sort(locDays);

    return TWeekdays{locDays, Weekdays->Repeat};
}

TMaybe<TWeekdaysAlarm> TWeekdaysAlarm::GetRest(const TInstant& now) const {
    if (!Weekdays) {
        if (Begin < now)
            return Nothing();
        return *this;
    }

    const auto& weekdays = *Weekdays;

    if (weekdays.Days.empty())
        return Nothing();

    if (weekdays.Repeat)
        return *this;

    auto curr = ToUTC(Begin);

    TInstant begin;
    TVector<NDatetime::TWeekday> days;

    for (int i = 0; i < NUM_WEEKDAYS; ++i, curr = NDatetime::AddDays(curr, 1)) {
        const auto day = GetWeekday(curr);
        if (weekdays.Contains(day) && FromUTC(curr) >= now) {
            if (days.empty())
                begin = FromUTC(curr);
            days.push_back(day);
        }
    }

    if (days.empty())
        return Nothing();

    return TWeekdaysAlarm{begin, days, false /* repeat */};
}

bool TWeekdaysAlarm::IsSubsetOf(const TWeekdaysAlarm& alarm) const {
    if (!IsRegular() && !alarm.IsRegular()) {
        const auto thisTriggers = GetNonRegularAlarmTriggers();
        const auto alarmTriggers = alarm.GetNonRegularAlarmTriggers();
        return Includes(alarmTriggers, thisTriggers);
    }

    // Regular alarm can't be a subset of a non-regular alarm.
    if (IsRegular() && !alarm.IsRegular())
        return false;

    if (!IsRegular() && alarm.IsRegular()) {
        const auto& weekdays = *alarm.Weekdays;
        const auto triggers = GetNonRegularAlarmTriggers();

        for (const auto& trigger : triggers) {
            if (!SameDayTime(trigger, alarm.Begin))
                return false;
            const auto day = GetWeekday(ToUTC(trigger));
            if (!weekdays.Contains(day))
                return false;
        }

        return true;
    }

    Y_ASSERT(IsRegular() && alarm.IsRegular());

    if (!SameDayTime(Begin, alarm.Begin))
        return false;

    auto thisDays = Weekdays->Days;
    Sort(thisDays);

    auto alarmDays = alarm.Weekdays->Days;
    Sort(alarmDays);

    return Includes(alarmDays, thisDays);
}

bool TWeekdaysAlarm::IsOutdated(const TInstant& now) const {
    return !GetRest(now);
}

bool TWeekdaysAlarm::IsRegular() const {
    return Weekdays && !Weekdays->Days.empty() && Weekdays->Repeat;
}

bool TWeekdaysAlarm::IsEmpty() const {
    return Weekdays && Weekdays->Days.empty();
}

bool TWeekdaysAlarm::TriggersOnDayTime(const TDayTime& dayTime, const NDatetime::TCivilSecond& now,
                                       const NDatetime::TTimeZone& tz) const {
    const auto trigger = NDatetime::Convert(Begin, tz);
    const auto time = dayTime.Apply(now);

    if (SameDayTime(trigger, time))
        return true;
    if (dayTime.AmbiguousHours() && SameDayTime(trigger, NDatetime::AddHours(time, 12)))
        return true;

    return false;
}

bool TWeekdaysAlarm::TriggersOnDayTime(const TMaybe<TDayTime>& dayTime, const NDatetime::TCivilSecond& now,
                                       const NDatetime::TTimeZone& tz) const {
    return dayTime ? TriggersOnDayTime(*dayTime, now, tz) : true;
}

bool TWeekdaysAlarm::TriggersOnlyOnDate(const TDate& date, const NDatetime::TCivilSecond& now,
                                        const NDatetime::TTimeZone& tz) const {
    if (IsRegular())
        return false;

    const auto time = date.Apply(now);

    const auto triggers = GetNonRegularAlarmTriggers();
    return triggers.size() == 1 && SameDate(NDatetime::Convert(triggers[0], tz), time);
}

bool TWeekdaysAlarm::TriggersOnlyOnDate(const TMaybe<TDate>& date, const NDatetime::TCivilSecond& now,
                                        const NDatetime::TTimeZone& tz) const {
    return date ? TriggersOnlyOnDate(*date, now, tz) : true;
}

bool TWeekdaysAlarm::TriggersOnDate(const TDate& date, const NDatetime::TCivilSecond& now,
                                    const NDatetime::TTimeZone& tz) const {
    const auto time = date.Apply(now);
    const auto weekday = GetWeekday(time);

    if (IsRegular()) {
        if (TMaybe<TWeekdays> weekdays = GetLocalWeekdays(tz))
            return weekdays->Contains(weekday);

        Y_ASSERT(false);
        return false;
    }

    const auto triggers = GetNonRegularAlarmTriggers();
    for (const auto& trigger : triggers) {
        if (SameDate(NDatetime::Convert(trigger, tz), time))
            return true;
    }
    return false;
}

bool TWeekdaysAlarm::TriggersOnDate(const TMaybe<TDate>& date, const NDatetime::TCivilSecond& now,
                                    const NDatetime::TTimeZone& tz) const {
    return date ? TriggersOnDate(*date, now, tz) : true;
}

bool TWeekdaysAlarm::TriggersOnSameWeekdays(const TWeekdays& weekdays, const NDatetime::TTimeZone& tz) const {
    const auto triggerWeekdays = GetTriggerWeekdays(tz);

    // This check may seems paranoid, but this corresponds to the name
    // of the function.
    if (weekdays.Days.empty() && triggerWeekdays.empty())
        return true;

    if (weekdays.Repeat != IsRegular())
        return false;

    const auto thisDays = GetTriggerWeekdays(tz);

    auto days = weekdays.Days;
    SortUnique(days);

    return thisDays == days;
}

bool TWeekdaysAlarm::TriggersOnWeekdays(const TWeekdays& weekdays, const NDatetime::TTimeZone& tz) const {
    const auto triggerWeekdays = GetTriggerWeekdays(tz);

    auto days = weekdays.Days;
    SortUnique(days);

    return Includes(triggerWeekdays, days);
}

TVector<TInstant> TWeekdaysAlarm::GetNonRegularAlarmTriggers() const {
    Y_ASSERT(!IsRegular());

    TVector<TInstant> triggers;
    if (!Weekdays) {
        triggers.push_back(Begin);
        Y_ASSERT(IsSorted(triggers.begin(), triggers.end()));
        return triggers;
    }

    const auto& weekdays = *Weekdays;
    Y_ASSERT(!weekdays.Repeat);
    auto curr = ToUTC(Begin);
    for (int i = 0; i < NUM_WEEKDAYS; ++i, curr = NDatetime::AddDays(curr, 1)) {
        if (weekdays.Contains(GetWeekday(curr)))
            triggers.push_back(FromUTC(curr));
    }

    Y_ASSERT(IsSorted(triggers.begin(), triggers.end()));
    return triggers;
}

} // namespace NAlice::NScenarios::NAlarm

template <>
void Out<NAlice::NScenarios::NAlarm::TWeekdaysAlarm>(IOutputStream& o, const NAlice::NScenarios::NAlarm::TWeekdaysAlarm& alarm) {
    o << "TWeekdaysAlarm [";
    o << alarm.Begin;
    if (alarm.Weekdays)
        o << " " << *alarm.Weekdays;
    o << "]";
}
