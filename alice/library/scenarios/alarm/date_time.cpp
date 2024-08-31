#include "date_time.h"

#include "helpers.h"
#include "weekday.h"

#include <contrib/libs/cctz/include/cctz/civil_time_detail.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <util/datetime/base.h>
#include <util/generic/ylimits.h>
#include <util/stream/output.h>
#include <util/string/ascii.h>
#include <util/system/yassert.h>

using namespace NDatetime;

namespace NAlice::NScenarios::NAlarm {

TMaybe<TStringBuf> DayPartToPeriod(TStringBuf dayPart) {
    if (dayPart.StartsWith("night") || dayPart.StartsWith("morning"))
        return "am";
    if (dayPart == TStringBuf("day") || dayPart.StartsWith("evening"))
        return "pm";
    return Nothing();
}

bool IsPluralDayPart(const NSc::TValue& dayPart) {
    if (!dayPart.IsString())
        return false;

    const auto& value = dayPart.GetString();
    return value == "nights" || value == "mornings" || value == "evenings";
}

void AdjustTimeValue(NSc::TValue& time, const NSc::TValue& dayPart) {
    if (!time.IsDict() || !dayPart.IsString())
        return;

    auto period = DayPartToPeriod(dayPart.GetString());
    if (!period)
        return;

    const NSc::TValue& hours = time.TrySelect("hours");
    if (!hours.IsNull() && hours.IsIntNumber()) {
        i64 hoursValue = hours.GetIntNumber();
        if (hoursValue <= 12)
            time["period"].SetString(*period);
        if (hoursValue == 0)
            time["hours"].SetIntNumber(12);
    }
}

bool IsPluralDayPart(const TString& dayPart) {
    return dayPart == "nights" || dayPart == "mornings" || dayPart == "evenings";
}

void AdjustTimeValue(NJson::TJsonValue& time, const TString& dayPart) {
    if (!time.IsMap()) {
        return;
    }

    auto period = DayPartToPeriod(dayPart);
    if (!period) {
        return;
    }

    const auto& hours = time["hours"];
    if (!hours.IsNull() && hours.IsInteger()) {
        i64 hoursValue = hours.GetInteger();
        if (hoursValue <= 12) {
            time["period"].SetValue(NJson::TJsonValue(*period));
        }
        if (hoursValue == 0) {
            time["hours"].SetValue(NJson::TJsonValue(12));
        }
    }
}

namespace {
enum EParseStatus { Absent, Invalid, Success };

struct TKeys {
    constexpr TKeys(TStringBuf value, TStringBuf relative)
        : Value(value)
        , Relative(relative) {
    }

    const TStringBuf Value;
    const TStringBuf Relative;
};

class TStreamWrapper {
public:
    explicit TStreamWrapper(IOutputStream& os)
        : Stream(os) {
    }

    template <typename T>
    TStreamWrapper& operator<<(const T& value) {
        Stream << value;
        return *this;
    }

    template <typename T>
    void Emit(TStringBuf name, const T& value) {
        if (!First)
            Stream << ", ";
        Stream << name << ": " << value;
        First = false;
    }

private:
    IOutputStream& Stream;
    bool First = true;
};

constexpr TKeys KEYS_YEARS(TStringBuf("years"), TStringBuf("years_relative"));
constexpr TKeys KEYS_MONTHS(TStringBuf("months"), TStringBuf("months_relative"));
constexpr TKeys KEYS_DAYS(TStringBuf("days"), TStringBuf("days_relative"));
constexpr TKeys KEYS_WEEKS(TStringBuf("weeks"), TStringBuf("weeks_relative"));

constexpr TKeys KEYS_HOURS(TStringBuf("hours"), TStringBuf("hours_relative"));
constexpr TKeys KEYS_MINUTES(TStringBuf("minutes"), TStringBuf("minutes_relative"));
constexpr TKeys KEYS_SECONDS(TStringBuf("seconds"), TStringBuf("seconds_relative"));

constexpr TStringBuf KEY_PERIOD = "period";
constexpr TStringBuf KEY_WEEKDAY = "weekday";

constexpr TStringBuf PERIOD_AM = "am";
constexpr TStringBuf PERIOD_PM = "pm";

EParseStatus ParseInt(const NSc::TDict& dict, TStringBuf key, int min, int max, int& value) {
    Y_ASSERT(max > 0);

    const auto it = dict.find(key);
    if (it == dict.end())
        return EParseStatus::Absent;

    const NSc::TValue& content = it->second;
    if (!content.IsIntNumber())
        return EParseStatus::Invalid;

    const i64 v = content.GetIntNumber();
    if (v < min || v >= max)
        return EParseStatus::Invalid;

    value = static_cast<int>(v);
    return EParseStatus::Success;
}

EParseStatus ParseBool(const NSc::TDict& dict, TStringBuf key, bool& value) {
    const auto it = dict.find(key);
    if (it == dict.end())
        return EParseStatus::Absent;

    const NSc::TValue& content = it->second;
    if (!content.IsBool())
        return EParseStatus::Invalid;

    value = content.GetBool();
    return EParseStatus::Success;
}

bool ParseComponent(const NSc::TDict& dict, const TKeys& keys, int max, TMaybe<TDayTime::TComponent>& component) {
    bool relative;
    const auto rs = ParseBool(dict, keys.Relative, relative);

    if (rs == EParseStatus::Invalid)
        return false;

    if (rs == EParseStatus::Absent)
        relative = false;

    int value;
    const auto vs = ParseInt(dict, keys.Value, relative ? Min<int>() : 0, relative ? Max<int>() : max, value);

    if (vs == EParseStatus::Invalid)
        return false;

    if (vs == EParseStatus::Absent && rs == EParseStatus::Success)
        return false;

    if (vs == EParseStatus::Absent) {
        component.Clear();
        return true;
    }

    Y_ASSERT(vs == EParseStatus::Success);

    if (rs == EParseStatus::Absent)
        relative = false;
    Y_ASSERT(rs == EParseStatus::Success || rs == EParseStatus::Absent);

    component = TDayTime::TComponent(value, relative);
    return true;
}

/** Returns false if data is invalid. Otherwise true, even if weekday is absent */
bool ParseWeekdayFromJson(const NSc::TDict& dict, TMaybe<NDatetime::TWeekday>& weekday) {
    const auto it = dict.find(KEY_WEEKDAY);
    if (it == dict.end()) {
        weekday.Clear();
        return true;
    }

    NDatetime::TWeekday wd;
    if (!ParseWeekday(it->second, wd)) {
        return false;
    }

    weekday.ConstructInPlace(wd);
    return true;
}

bool ParsePeriod(const NSc::TDict& dict, TDayTime::EPeriod& period) {
    const auto it = dict.find(KEY_PERIOD);
    if (it == dict.end()) {
        period = TDayTime::EPeriod::Unspecified;
        return true;
    }

    const NSc::TValue& value = it->second;
    if (!value.IsString())
        return false;

    if (value == PERIOD_AM) {
        period = TDayTime::EPeriod::AM;
        return true;
    }
    if (value == PERIOD_PM) {
        period = TDayTime::EPeriod::PM;
        return true;
    }

    return false;
}

void SerializeWeekday(const TMaybe<NDatetime::TWeekday>& weekday, NSc::TDict& dict) {
    if (!weekday)
        return;

    dict[KEY_WEEKDAY].SetIntNumber(GetWeekdayIndex(*weekday));
}

void SerializeComponent(const TKeys& keys, const TMaybe<TDayTime::TComponent>& component, NSc::TDict& dict) {
    if (!component)
        return;

    dict[keys.Value].SetIntNumber(component->Value);

    if (component->Relative)
        dict[keys.Relative].SetBool(component->Relative);
}

// TODO (@vi002, @petrk): this should be moved to
// library/cpp/timezone_conversion/civil.h
TCivilSecond SetYears(const TCivilSecond& s, long years) {
    return TCivilSecond(years, s.month(), s.day(), s.hour(), s.minute(), s.second());
}

TCivilSecond SetMonths(const TCivilSecond& s, int months) {
    return TCivilSecond(s.year(), months, s.day(), s.hour(), s.minute(), s.second());
}

TCivilSecond SetDays(const TCivilSecond& s, int days) {
    return TCivilSecond(s.year(), s.month(), days, s.hour(), s.minute(), s.second());
}

TCivilSecond SetHours(const TCivilSecond& s, int hours) {
    return TCivilSecond(s.year(), s.month(), s.day(), hours, s.minute(), s.second());
}

TCivilSecond SetMinutes(const TCivilSecond& s, int minutes) {
    return TCivilSecond(s.year(), s.month(), s.day(), s.hour(), minutes, s.second());
}

TCivilSecond SetSeconds(const TCivilSecond& s, int seconds) {
    return TCivilSecond(s.year(), s.month(), s.day(), s.hour(), s.minute(), seconds);
}

TCivilSecond SetCivil(const TCivilSecond& s, ECivilUnit unit, int value) {
    switch (unit) {
        case ECivilUnit::Second:
            return SetSeconds(s, value);

        case ECivilUnit::Minute:
            return SetMinutes(s, value);

        case ECivilUnit::Hour:
            return SetHours(s, value);

        case ECivilUnit::Day:
            return SetDays(s, value);

        case ECivilUnit::Month:
            return SetMonths(s, value);

        case ECivilUnit::Year:
            return SetYears(s, value);
    }

    Y_ASSERT(false);
    return s;
}

void From24Hours(int hours, TMaybe<TDayTime::TComponent>& toHours, TDayTime::EPeriod& toPeriod) {
    if (hours == 0) {
        toHours.ConstructInPlace(12, false /* relative */);
        toPeriod = TDayTime::EPeriod::AM;
    } else if (hours == 12) {
        toHours.ConstructInPlace(hours, false /* relative */);
        toPeriod = TDayTime::EPeriod::PM;
    } else if (hours < 12) {
        toHours.ConstructInPlace(hours, false /* relative */);
        toPeriod = TDayTime::EPeriod::AM;
    } else {
        toHours.ConstructInPlace(hours - 12, false /* relative */);
        toPeriod = TDayTime::EPeriod::PM;
    }
}

int To24Hours(int hours, TDayTime::EPeriod period) {
    switch (period) {
        case TDayTime::EPeriod::AM:
            if (hours == 12) {
                // 12:01 am == 00:01
                hours = 0;
            }
            break;
        case TDayTime::EPeriod::PM:
            if (hours != 12) {
                // 12:01 pm == 12:01
                // 1:00 pm == 13:00
                hours += 12;
            }
            break;
        case TDayTime::EPeriod::Unspecified:
            break;
    }
    return hours;
}

TCivilSecond Ap(const TCivilSecond& now, ECivilUnit unit, const TDateTimeComponent& component,
                TDayTime::EPeriod period = TDayTime::EPeriod::Unspecified) {
    int value = component.Value;
    if (unit == ECivilUnit::Hour && !component.Relative)
        value = To24Hours(value, period);

    return component.Relative ? AddCivil(now, TCivilDiff(value, unit)) : SetCivil(now, unit, value);
}

TCivilSecond Ap(const TCivilSecond& now, ECivilUnit unit, const TMaybe<TDateTimeComponent>& component, bool relative,
                TDayTime::EPeriod period = TDayTime::EPeriod::Unspecified) {
    if (component)
        return Ap(now, unit, *component, period);

    switch (unit) {
        case ECivilUnit::Year:
            [[fallthrough]];
        case ECivilUnit::Month:
            [[fallthrough]];
        case ECivilUnit::Day: {
            const TDateTimeComponent def(relative ? 0 : 1, relative);
            return Ap(now, unit, def, period);
        }
        case ECivilUnit::Hour: {
            if (relative)
                return now;
            const TDateTimeComponent def(12, false /* relative */);
            return Ap(now, unit, def, TDayTime::EPeriod::AM);
        }
        case ECivilUnit::Minute:
            [[fallthrough]];
        case ECivilUnit::Second: {
            const TDateTimeComponent def(0, relative);
            return Ap(now, unit, def, period);
        }
    }

    Y_ASSERT(false);
    return now;
}

template <int Digits>
bool ParseInt(TStringBuf& buffer, int& value) {
    static_assert(Digits >= 0 && Digits < 10, "");

    if (buffer.size() < Digits)
        return false;

    int v = 0;
    for (size_t i = 0; i < Digits; ++i) {
        const auto c = buffer[i];
        if (!IsAsciiDigit(c))
            return false;
        v = v * 10 + (c - '0');
    }

    buffer.Skip(Digits);
    value = v;
    return true;
}
} // namespace

// TDayTime --------------------------------------------------------------------
TDayTime::TDayTime(const TCivilSecond& time)
    : Minutes(TComponent(time.minute(), false /* relative */))
    , Seconds(TComponent(time.second(), false /* relative */)) {
    From24Hours(time.hour(), Hours, Period);
}

// static
TMaybe<TDayTime> TDayTime::FromValue(const NSc::TValue& value) {
    const auto& dict = value.GetDict();

    TDayTime dayTime;
    if (!ParseComponent(dict, KEYS_HOURS, MAX_HOURS, dayTime.Hours))
        return Nothing();
    if (!ParseComponent(dict, KEYS_MINUTES, MAX_MINUTES, dayTime.Minutes))
        return Nothing();
    if (!ParseComponent(dict, KEYS_SECONDS, MAX_SECONDS, dayTime.Seconds))
        return Nothing();
    if (!ParsePeriod(dict, dayTime.Period))
        return Nothing();

    switch (dayTime.Period) {
        case EPeriod::AM:
            [[fallthrough]];
        case EPeriod::PM:
            // No sense in am/pm when main time component is relative or no time at all.
            if (dayTime.IsRelative() || dayTime.IsEmpty())
                return Nothing();

            // When hours are not specified, correct hour value must
            // be set to 12, not 0.  So, "12 minutes am" will become
            // "12:12 am", as expected.
            if (!dayTime.Hours)
                dayTime.Hours = TComponent(12 /* value */, false /* relative */);

            if (dayTime.Hours->Value > 12) {
                if (dayTime.Period == EPeriod::PM) // it is ok, since hours > 12 is always PM
                    dayTime.Period = EPeriod::Unspecified;
                else
                    return Nothing();
            }

            if (dayTime.Hours->Value == 0)
                return Nothing();

            break;
        case EPeriod::Unspecified:
            break;
    }

    if (dayTime.IsEmpty())
        return Nothing();

    return dayTime;
}

NSc::TValue TDayTime::ToValue() const {
    NSc::TValue value;
    auto& dict = value.GetDictMutable();

    SerializeComponent(KEYS_HOURS, Hours, dict);
    SerializeComponent(KEYS_MINUTES, Minutes, dict);
    SerializeComponent(KEYS_SECONDS, Seconds, dict);

    switch (Period) {
        case EPeriod::AM:
            dict[KEY_PERIOD] = PERIOD_AM;
            break;
        case EPeriod::PM:
            dict[KEY_PERIOD] = PERIOD_PM;
            break;
        case EPeriod::Unspecified:
            break;
    }

    return value;
}

TCivilSecond TDayTime::Apply(const TCivilSecond& now) const {
    auto result = now;

    if (IsEmpty())
        return result;

    const bool relative = IsRelative();
    result = Ap(result, ECivilUnit::Hour, Hours, relative, Period);
    result = Ap(result, ECivilUnit::Minute, Minutes, relative, Period);
    result = Ap(result, ECivilUnit::Second, Seconds, relative, Period);

    return result;
}

bool TDate::IsToday(const NDatetime::TCivilSecond& now) const {
    return Apply(now) == now;
}

bool TDate::IsTomorrow(const NDatetime::TCivilSecond& now) const {
    return Apply(now) == AddDays(now, 1);
}

bool TDate::IsTodayOrTomorrow(const NDatetime::TCivilSecond& now) const {
    return IsToday(now) || IsTomorrow(now);
}

bool TDate::IsTomorrow() const {
    return *this == MakeTomorrow();
}

bool TDayTime::IsEmpty() const {
    return !Hours && !Minutes && !Seconds;
}

bool TDayTime::IsRelative() const {
    if (Hours)
        return Hours->Relative;
    if (Minutes)
        return Minutes->Relative;
    if (Seconds)
        return Seconds->Relative;
    return false;
}

// TDate -----------------------------------------------------------------------
// static
TDate TDate::MakeToday() {
    return TDate(Nothing() /* years */, Nothing() /* months */, MakeMaybe<TComponent>(0, true) /* days */);
}

// static
TDate TDate::MakeTomorrow() {
    return TDate(Nothing() /* years */, Nothing() /* months */, MakeMaybe<TComponent>(1, true) /* days */);
}

// static
TMaybe<TDate> TDate::FromValue(const NSc::TValue& value) {
    // TODO (@vi002, @petrk): this code duplicates
    // TDayTime::FromValue, need to get rid of this duplication.

    const auto& dict = value.GetDict();

    // ' + 1' in ParseComponent is needed because upperbound is exclusive.
    TDate date;
    if (!ParseComponent(dict, KEYS_YEARS, MAX_YEARS + 1, date.Years))
        return Nothing();
    if (!ParseComponent(dict, KEYS_MONTHS, MAX_MONTHS + 1, date.Months))
        return Nothing();
    if (!ParseComponent(dict, KEYS_DAYS, MAX_DAYS + 1, date.Days))
        return Nothing();
    if (!ParseComponent(dict, KEYS_WEEKS, MAX_WEEKS + 1, date.Weeks))
        return Nothing();

    if (!ParseWeekdayFromJson(dict, date.Weekday))
        return Nothing();

    if (date.Weeks) {
        // Non relative week has not supported yet.
        // TODO add when we understand how to do it!
        if (!date.Weeks->Relative)
            return Nothing();

        // It looks strange if both YMD and weeks are specified.
        // (i.e. 'первое апреля через две недели')
        // So don't support it right now.
        if (date.Years || date.Months || date.Days)
            return Nothing();
    }

    return date;
}

NSc::TValue TDate::ToValue() const {
    NSc::TValue value;
    auto& dict = value.GetDictMutable();

    SerializeComponent(KEYS_YEARS, Years, dict);
    SerializeComponent(KEYS_MONTHS, Months, dict);
    SerializeComponent(KEYS_DAYS, Days, dict);
    SerializeComponent(KEYS_WEEKS, Weeks, dict);
    SerializeWeekday(Weekday, dict);

    return value;
}

NDatetime::TCivilSecond TDate::Apply(const NDatetime::TCivilSecond& now) const {
    // Following comment specifies what to do with missed parts of the date.
    //
    // '*' means unspecified, 'A' means absolute, 'R' means relative,
    // first character denotes year, second denotes month and the last
    // denotes day:
    //
    // **A - we need to keep current year and month
    // *AA - we need to keep current year
    // *RA - we need to keep current year
    // A*A - set month to 1
    // R*A - keep current month
    // **R - we need to keep current year and month
    // *AR - we need to keep current year
    // *RR - we need to keep current year
    // A*R - we need to keep current month
    // R*R - we need to keep current month
    //
    // This means that the only case when we need to set month to 1,
    // is when year and day are absolute, and month is missed.

    // TODO (@vi002, @petrk): this code duplicates TDayTime::Apply,
    // need to get rid of this duplication.

    auto result = now;

    if (Weeks) {
        // Weeks must be always relative (it is checked in FromValue()).
        Y_ASSERT(Weeks->Relative);

        if (Weekday) {
            const NDatetime::TCivilDay day{NDatetime::WeekdayOnTheWeek(NDatetime::TCivilDay{result}, NDatetime::TWeekday::monday)};
            result = SetYears(result, day.year());
            result = SetMonths(result, day.month());
            result = SetDays(result, day.day());
        }

        result = NDatetime::AddDays(result, Weeks->Value * 7);
    }

    if (Weekday) {
        // XXX (@vi002, @petrk) respect weeks (i.e.: monday of the next week, next monday)
        // it also looses the information about days, months and years (i.e.: first monday of may)
        // but it is a really rare case
        const NDatetime::TCivilDay day{NDatetime::NearestWeekday(NDatetime::TCivilDay{result}, *Weekday)};
        result = SetYears(result, day.year());
        result = SetMonths(result, day.month());
        result = SetDays(result, day.day());
        return result;
    }

    if (!Days)
        return result;

    if (Years)
        result = Ap(result, ECivilUnit::Year, *Years);

    if (Months && Months->Relative) {
        result = Ap(result, ECivilUnit::Month, *Months);
        result = Ap(result, ECivilUnit::Day, *Days);
    } else if (Months && !Months->Relative) {
        // We should set days before months here! It's impossible to set month to june on may 31.
        result = Ap(result, ECivilUnit::Day, *Days);
        result = Ap(result, ECivilUnit::Month, *Months);
    } else if (Years && !Years->Relative && Days && !Days->Relative) {
        // The only case when we need to set month to 1.
        result = Ap(result, ECivilUnit::Month, TComponent(1, false /* relative */));
        result = Ap(result, ECivilUnit::Day, *Days);
    } else {
        result = Ap(result, ECivilUnit::Day, *Days);
    }

    return result;
}

bool TDate::HasExactDay() const {
    return Days || Weekday || Weeks;
}

bool TDate::HasOnlyWeekday() const {
    return Weekday && !Years && !Months && !Days && !Weeks;
}

} // namespace NAlice::NScenarios::NAlarm

template <>
void Out<NAlice::NScenarios::NAlarm::TDateTimeComponent>(IOutputStream& o,
                                                         NAlice::NScenarios::NAlarm::TDateTimeComponent const& component) {
    o << component.Value << " ";
    if (component.Relative)
        o << "relative";
    else
        o << "absolute";
}

template <>
void Out<NAlice::NScenarios::NAlarm::TDayTime>(IOutputStream& o, NAlice::NScenarios::NAlarm::TDayTime const& dayTime) {
    o << "DayTime [";
    if (dayTime.Hours)
        o << "Hours: " << *dayTime.Hours << ", ";
    if (dayTime.Minutes)
        o << "Minutes: " << *dayTime.Minutes << ", ";
    if (dayTime.Seconds)
        o << "Seconds: " << *dayTime.Seconds << ", ";
    o << "Period: " << dayTime.Period;
    o << "]";
}

template <>
void Out<NAlice::NScenarios::NAlarm::TDate>(IOutputStream& o, NAlice::NScenarios::NAlarm::TDate const& date) {
    NAlice::NScenarios::NAlarm::TStreamWrapper s{o};

    s << "Date [";
    if (date.Years)
        s.Emit("Years", *date.Years);
    if (date.Months)
        s.Emit("Months", *date.Months);
    if (date.Days)
        s.Emit("Days", *date.Days);
    if (date.Weekday)
        s.Emit("Weekday: ", *date.Weekday);
    s << "]";
}
