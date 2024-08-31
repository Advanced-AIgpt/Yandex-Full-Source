#pragma once

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/timezone_conversion/civil.h>

#include <util/generic/maybe.h>
#include <util/generic/vector.h>

namespace NAlice::NScenarios::NAlarm {

inline constexpr TStringBuf WEEKDAYS_KEY_REPEAT = "repeat";
inline constexpr TStringBuf WEEKDAYS_KEY_WEEKDAYS = "weekdays";

struct TWeekdays {
    TWeekdays() = default;

    TWeekdays(const TVector<NDatetime::TWeekday>& days, bool repeat)
        : Days(days)
        , Repeat(repeat) {
    }

    static TMaybe<TWeekdays> FromValue(const NSc::TValue& value);
    NSc::TValue ToValue() const;

    bool operator==(const TWeekdays& rhs) const {
        return Days == rhs.Days && Repeat == rhs.Repeat;
    }

    bool Contains(const NDatetime::TWeekday& weekday) const;

    bool Empty() const {
        return Days.empty();
    }

    // List of weekdays alarm should be triggered.
    TVector<NDatetime::TWeekday> Days;

    // True if it's needed to repeat alarm weekly.
    bool Repeat = false;
};

} // namespace NAlice::NScenarios::NAlarm
