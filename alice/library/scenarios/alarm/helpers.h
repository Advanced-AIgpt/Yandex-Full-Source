#pragma once

#include "weekdays.h"
#include "weekdays_alarm.h"

#include <alice/library/calendar_parser/types.h>

#include <library/cpp/semver/semver.h>
#include <library/cpp/timezone_conversion/civil.h>

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/generic/noncopyable.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/system/types.h>
#include <util/system/yassert.h>

namespace NAlice::NScenarios::NAlarm {

constexpr TStringBuf SLOT_TYPE_TIME = "time";

TInstant ToInstant(const NDatetime::TCivilSecond& now, const NDatetime::TTimeZone& tz);
TInstant FromUTC(const NDatetime::TCivilSecond& now);
NDatetime::TCivilSecond ToUTC(const TInstant& now);
NDatetime::TCivilSecond ToUTC(const NDatetime::TCivilSecond& now, const NDatetime::TTimeZone& tz);
NDatetime::TWeekday GetWeekday(const NDatetime::TCivilSecond& now);

bool SameDayTime(const NDatetime::TCivilSecond& a, const NDatetime::TCivilSecond& b);
bool SameDayTime(const TInstant& a, const TInstant& b);

bool SameDate(const NDatetime::TCivilSecond& a, const NDatetime::TCivilSecond& b);
bool SameDate(const TInstant& a, const TInstant& b);

bool IsMidnight(const NDatetime::TCivilSecond& a);

// Returns true when |a| corresponds to the range between 12:00am and
// 4:00am.  This range has a special meaning, because when current
// time is in the range and one says "set alarm on 6am tomorrow", she
// actually means "today", not "tomorrow".
bool IsCivilNight(const NDatetime::TCivilSecond& a);

NDatetime::TCivilSecond GetEventTime(const NDatetime::TCivilSecond& now, const TDayTime& dayTime);
NDatetime::TCivilSecond GetEventTime(const NDatetime::TCivilSecond& now, NDatetime::TCivilSecond event,
                                     const TDayTime& dayTime);
TMaybe<NDatetime::TCivilSecond> GetEventDateTime(const NDatetime::TCivilSecond& now, const TDayTime& dayTime,
                                                 const TDate& date);
TMaybe<NDatetime::TCivilSecond> GetEventDateTime(const NDatetime::TCivilSecond& now, const TDayTime& dayTime,
                                                 const TMaybe<TDate>& date);

// Computes date-time when alarm must be triggered.  This is the time
// on the current day, if |dayTime| is not passed, or the time on the
// next day otherwise.
NDatetime::TCivilSecond GetAlarmTime(const NDatetime::TCivilSecond& now, const TDayTime& dayTime);
TWeekdaysAlarm GetAlarmTime(const TInstant& now, const NDatetime::TTimeZone& tz, const TDayTime& dayTime);

// Computes date-time when alarm must be triggered. This is the time
// on the current day, or the time on the next day (it depends on the
// |dayTime| + |date| combination).  If required |dayTime| + |date|
// combination is already passed or at least 24 hours away from the
// current time point, returns Nothing.

TMaybe<NDatetime::TCivilSecond> GetAlarmDateTime(const NDatetime::TCivilSecond& now, const TDayTime& dayTime,
                                                 const TDate& date);
TMaybe<TWeekdaysAlarm> GetAlarmDateTime(const TInstant& now, const NDatetime::TTimeZone& tz, const TDayTime& dayTime,
                                        const TDate& date);

// Calls GetAlarmDateTime() if date exists, calls GetAlarmTime() otherwise.
TMaybe<NDatetime::TCivilSecond> GetAlarmDateTime(const NDatetime::TCivilSecond& now, const TDayTime& dayTime,
                                                 const TMaybe<TDate>& date);

// Computes the first date-time when alarm must be triggered and
// corresponding weekdays, in UTC.
TWeekdaysAlarm GetAlarmWeekdays(const NDatetime::TCivilSecond& now, const NDatetime::TTimeZone& tz,
                                const TDayTime& dayTime, const TWeekdays& weekdays);
TWeekdaysAlarm GetAlarmWeekdays(const TInstant& now, const NDatetime::TTimeZone& tz, const TDayTime& dayTime,
                                const TWeekdays& weekdays);

// Creates time as 24 hours in time slot format.
NSc::TValue TimeToValue(const TInstant& now, const NDatetime::TTimeZone& tz);
NSc::TValue TimeToValue(const NDatetime::TCivilSecond& time);
// Creates date as today/tomorrow or specific date in date slot format.
NSc::TValue DateToValue(const NDatetime::TCivilSecond& now, const TDate& date);
NSc::TValue DateToValue(const NDatetime::TCivilSecond& now, const NDatetime::TCivilSecond& date);

bool MatchesExactlyDayTimeAndDate(
    const TWeekdaysAlarm& alarm,
    const TMaybe<TDayTime>& dayTime,
    const TMaybe<TDate>& date,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz
);

bool MatchesApproximatelyDayTimeAndDate(
    const TWeekdaysAlarm& alarm,
    const TMaybe<TDayTime>& dayTime,
    const TMaybe<TDate>& date,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz
);

bool MatchesExactlyDayTimeAndWeekdays(
    const TWeekdaysAlarm& alarm,
    const TMaybe<TDayTime>& dayTime,
    const TWeekdays& weekdays,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz
);

bool MatchesApproximatelyDayTimeAndWeekdays(
    const TWeekdaysAlarm& alarm,
    const TMaybe<TDayTime>& dayTime,
    const TWeekdays& weekdays,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz
);

bool MatchesApproximatelyDayTime(
    const TWeekdaysAlarm& alarm,
    const TDayTime& dayTime,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz
);

bool IsFirstBeforeSecond(const TDayTime& time1, const TDayTime& time2);

bool GetDistInTime(const TDayTime& time1, const TDayTime& time2, TDayTime& answer);

// Throws yexception if |indices| are invalid.  Validity of |indices|
// means that they're sorted, unique and all in [0, |values|.size()).
template <typename T>
TVector<T> RemoveByIndices(const TVector<T>& values, const TVector<size_t>& indices) {
    TVector<T> result;

    size_t i = 0;
    Y_ASSERT(i <= values.size());

    for (size_t j = 0; j < indices.size(); ++j) {
        Y_ASSERT((j == 0 && i == 0) || (j != 0 && i == indices[j - 1] + 1));

        if (indices[j] < i) {
            Y_ASSERT(j != 0 && indices[j] <= indices[j - 1]);
            ythrow yexception() << "Invalid indices";
        }

        const size_t index = indices[j];

        Y_ASSERT(i <= std::min(values.size(), index));
        for (; i < std::min(values.size(), index); ++i)
            result.push_back(values[i]);
        Y_ASSERT(i == std::min(values.size(), index));

        if (i == values.size()) {
            Y_ASSERT(index >= values.size());
            ythrow yexception() << "Invalid indices";
        }

        Y_ASSERT(i == index && index < values.size());
        ++i;
    }

    Y_ASSERT(i <= values.size());
    result.insert(result.end(), values.begin() + i, values.end());

    return result;
}

// Returns true if every element from the sorted range [begin2, end2)
// is found within the sorted range [begin1, end1).
template <typename TIt1, typename TIt2>
bool Includes(TIt1 begin1, TIt1 end1, TIt2 begin2, TIt2 end2) {
    Y_ASSERT(IsSorted(begin1, end1));
    Y_ASSERT(IsSorted(begin2, end2));

    while (begin2 != end2) {
        while (begin1 != end1 && *begin1 < *begin2)
            ++begin1;
        if (begin1 == end1 || *begin1 != *begin2)
            return false;
        ++begin2;
    }

    return true;
}

template <typename TCont1, typename TCont2>
bool Includes(const TCont1& cont1, const TCont2& cont2) {
    return Includes(std::begin(cont1), std::end(cont1), std::begin(cont2), std::end(cont2));
}

inline TDate::TComponent After(int value) {
    return {value, true /* relative */};
}

inline TDate::TComponent On(int value) {
    return {value, false /* relative */};
}

} // namespace NAlice::NScenarios::NAlarm
