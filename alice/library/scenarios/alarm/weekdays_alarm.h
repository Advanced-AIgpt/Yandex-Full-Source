#pragma once

#include "date_time.h"
#include "weekdays.h"

#include <alice/library/calendar_parser/parser.h>
#include <alice/library/calendar_parser/types.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NScenarios::NAlarm {

struct TWeekdaysAlarm {
    TWeekdaysAlarm() = default;

    explicit TWeekdaysAlarm(const TInstant& begin)
        : Begin(begin) {
    }

    TWeekdaysAlarm(const TInstant& begin, const TVector<NDatetime::TWeekday>& days, bool repeat)
        : Begin(begin)
        , Weekdays(TWeekdays{days, repeat}) {
    }

    static TMaybe<TWeekdaysAlarm> FromEvent(const NDatetime::TTimeZone& tz, const NCalendarParser::TEvent& event);
    void Append(NCalendarParser::TCalendar& calendar, TVector<TString>& lines) const;

    TString ToICalendar() const;

    // Throws NCalendarParser::TParser::TException in case of parsing
    // errors.  Also, returns empty list of alarms when |data| is
    // empty. Timezone is needed to adjust date/time components
    // without specified timezone.
    static TVector<TWeekdaysAlarm> FromICalendar(const NDatetime::TTimeZone& tz, TStringBuf data);

    static TString ToICalendar(const TVector<TWeekdaysAlarm>& alarms);
    static NSc::TValue ToICalendarPayload(const TVector<TWeekdaysAlarm>& alarms, const bool listeningIsPossible = false);

    bool operator==(const TWeekdaysAlarm& rhs) const {
        return Begin == rhs.Begin && Weekdays == rhs.Weekdays;
    }

    // For regular alarms, returns sorted list of trigger
    // weekdays. Otherwise, returns weekdays for trigger times. In any
    // case, result will be sort-uniqued.
    TVector<NDatetime::TWeekday> GetTriggerWeekdays(const NDatetime::TTimeZone& tz) const;

    // Returns TWeekdays where list of weekdays is adjusted according
    // to |tz|.
    TMaybe<TWeekdays> GetLocalWeekdays(const NDatetime::TTimeZone& tz) const;

    // For single-shot alarms returns them as is if they're not
    // passed, nothing otherwise.  For regular alarms returns them as
    // is.  For single-week alarms returns only non-passed days, and
    // nothing if all days were passed.
    TMaybe<TWeekdaysAlarm> GetRest(const TInstant& now) const;

    bool IsSubsetOf(const TWeekdaysAlarm& alarm) const;

    // Returns true if alarm won't be triggered since |now|.
    bool IsOutdated(const TInstant& now) const;

    // Returns true when alarm is regular.
    bool IsRegular() const;

    // Returns true when alarm does not correspond to any date-time.
    bool IsEmpty() const;

    bool TriggersOnDayTime(const TDayTime& dayTime, const NDatetime::TCivilSecond& now,
                           const NDatetime::TTimeZone& tz) const;
    bool TriggersOnDayTime(const TMaybe<TDayTime>& dayTime, const NDatetime::TCivilSecond& now,
                           const NDatetime::TTimeZone& tz) const;

    bool TriggersOnlyOnDate(const TDate& date, const NDatetime::TCivilSecond& now,
                            const NDatetime::TTimeZone& tz) const;
    bool TriggersOnlyOnDate(const TMaybe<TDate>& date, const NDatetime::TCivilSecond& now,
                            const NDatetime::TTimeZone& tz) const;

    bool TriggersOnDate(const TDate& date, const NDatetime::TCivilSecond& now, const NDatetime::TTimeZone& tz) const;
    bool TriggersOnDate(const TMaybe<TDate>& date, const NDatetime::TCivilSecond& now,
                        const NDatetime::TTimeZone& tz) const;

    bool TriggersOnSameWeekdays(const TWeekdays& weekdays, const NDatetime::TTimeZone& tz) const;

    bool TriggersOnWeekdays(const TWeekdays& weekdays, const NDatetime::TTimeZone& tz) const;

    TInstant Begin;
    TMaybe<TWeekdays> Weekdays;

private:
    // This function must be called only for non-regular
    // alarms. Return sort-uniqued list of trigger times.
    TVector<TInstant> GetNonRegularAlarmTriggers() const;
};

} // namespace NAlice::NScenarios::NAlarm
