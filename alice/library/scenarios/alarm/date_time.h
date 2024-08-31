#pragma once

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/timezone_conversion/civil.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/ysaveload.h>

namespace NAlice::NScenarios::NAlarm {

TMaybe<TStringBuf> DayPartToPeriod(TStringBuf dayPart);

bool IsPluralDayPart(const NSc::TValue& dayPart);

void AdjustTimeValue(NSc::TValue& time, const NSc::TValue& dayPart);

bool IsPluralDayPart(const TString& dayPart);

void AdjustTimeValue(NJson::TJsonValue& time, const TString& dayPart);

struct TDateTimeComponent {
    TDateTimeComponent() = default;

    TDateTimeComponent(int value, bool relative)
        : Value(value)
        , Relative(relative) {
    }

    bool operator==(const TDateTimeComponent& rhs) const {
        return Value == rhs.Value && Relative == rhs.Relative;
    }

    int Value = 0;
    bool Relative = false;
};

struct TDayTime {
    using TComponent = TDateTimeComponent;

    enum class EPeriod { AM, PM, Unspecified };

    // TODO (@vi002, @petrk): relative values may have larger limits.
    static constexpr int MAX_HOURS = 24;
    static constexpr int MAX_MINUTES = 60;
    static constexpr int MAX_SECONDS = 60;

    TDayTime() = default;

    TDayTime(const TMaybe<TComponent>& hours, const TMaybe<TComponent>& minutes, const TMaybe<TComponent>& seconds,
             EPeriod period)
        : Hours(hours)
        , Minutes(minutes)
        , Seconds(seconds)
        , Period(period) {
    }

    TDayTime(int hours, int minutes, int seconds, EPeriod period)
        : Hours(TComponent(hours, false /* relative */))
        , Minutes(TComponent(minutes, false /* relative */))
        , Seconds(TComponent(seconds, false /* relative */))
        , Period(period) {
    }

    explicit TDayTime(const NDatetime::TCivilSecond& time);

    static TMaybe<TDayTime> FromValue(const NSc::TValue& value);
    NSc::TValue ToValue() const;

    NDatetime::TCivilSecond Apply(const NDatetime::TCivilSecond& now) const;

    // Returns true when all time components are not set.
    bool IsEmpty() const;

    // Returns true when the most significant set component is relative.
    bool IsRelative() const;

    bool operator==(const TDayTime& rhs) const {
        return Hours == rhs.Hours && Minutes == rhs.Minutes && Seconds == rhs.Seconds && Period == rhs.Period;
    }

    bool AmbiguousHours() const {
        return Period == EPeriod::Unspecified && Hours && !Hours->Relative && Hours->Value >= 1 && Hours->Value <= 12;
    }

    bool HasNegative() const {
        return Hours && Hours->Value < 0
            || Minutes && Minutes->Value < 0
            || Seconds && Seconds->Value < 0;
    }

    bool HasRelativeNegative() const {
        return IsRelative() && HasNegative();
    }

    TMaybe<TComponent> Hours;
    TMaybe<TComponent> Minutes;
    TMaybe<TComponent> Seconds;

    EPeriod Period = EPeriod::Unspecified;
};

struct TDate {
    using TComponent = TDateTimeComponent;

    // TODO (@vi002, @petrk): relative values may have larger limits.
    static constexpr int MAX_YEARS = 9999;
    static constexpr int MAX_MONTHS = 12;
    static constexpr int MAX_DAYS = 31;
    static constexpr int MAX_WEEKS = 9999;

    TDate() = default;

    explicit TDate(const NDatetime::TCivilSecond& now)
        : TDate(now.year(), now.month(), now.day()) {
    }

    TDate(const TMaybe<TComponent>& years, const TMaybe<TComponent>& months, const TMaybe<TComponent>& days)
        : Years(years)
        , Months(months)
        , Days(days) {
    }

    TDate(int years, int months, int days)
        : Years(TComponent(years, false /* relative */))
        , Months(TComponent(months, false /* relative */))
        , Days(TComponent(days, false /* relative */)) {
    }

    static TDate MakeToday();
    static TDate MakeTomorrow();

    // It doesn't check if created TDate has days.
    static TMaybe<TDate> FromValue(const NSc::TValue& value);
    NSc::TValue ToValue() const;

    // Check if date has either day, weekday or week which allows us
    // to calculate the exact date in Apply().
    bool HasExactDay() const;

    bool HasOnlyWeekday() const;

    NDatetime::TCivilSecond Apply(const NDatetime::TCivilSecond& now) const;

    bool IsToday(const NDatetime::TCivilSecond& now) const;
    bool IsTomorrow(const NDatetime::TCivilSecond& now) const;
    bool IsTodayOrTomorrow(const NDatetime::TCivilSecond& now) const;

    bool IsTomorrow() const;

    bool operator==(const TDate& rhs) const {
        return Years == rhs.Years && Months == rhs.Months && Days == rhs.Days && Weeks == rhs.Weeks &&
               Weekday == rhs.Weekday;
    }

    TMaybe<TComponent> Years;
    TMaybe<TComponent> Months;
    TMaybe<TComponent> Days;
    TMaybe<TComponent> Weeks;
    TMaybe<NDatetime::TWeekday> Weekday;
};

class TISO8601SerDes final {
public:
    enum class ETimeZone { UTC, Unspecified };

    struct TDateTime {
        TDateTime() = default;

        TDateTime(const NDatetime::TCivilSecond& time, ETimeZone timeZone)
            : Time(time)
            , TimeZone(timeZone) {
        }

        bool operator==(const TDateTime & rhs) const {
            return Time == rhs.Time && TimeZone == rhs.TimeZone;
        }

        NDatetime::TCivilSecond Time;
        ETimeZone TimeZone = ETimeZone::Unspecified;
    };

    struct TException : public yexception {};

    // Converts |time| to UTC and serializes to compact representation
    // in accordance with ISO8601.
    static TString Ser(const TInstant& epoch);
    static TString Ser(const NDatetime::TCivilSecond& second, const NDatetime::TTimeZone& tz);

    // Serializes to compact representation in accordance with ISO8601.
    static TString Ser(const NDatetime::TCivilSecond& second);

    // Parses serialized date. In case of issues, throws TException.
    // Otherwise, skips date part of the |stream| and returns parsed
    // date.
    static NDatetime::TCivilDay DesDate(TStringBuf& stream);

    // Parses serialized date/time. In case of issues, throws
    // TException.  Otherwise, skips date-time part of the |stream|
    // and returns parsed date.
    //
    // NOTE: this implementation does not handle leap seconds. They're
    // always transformed to 59 seconds.
    static TDateTime DesDateTime(TStringBuf& stream);
};

} // namespace NAlice::NScenarios::NAlarm
