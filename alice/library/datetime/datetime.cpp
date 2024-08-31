#include "datetime.h"

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/timezone_conversion/convert.h>
#include <library/cpp/timezone_conversion/civil.h>

#include <util/draft/datetime.h>
#include <util/generic/map.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/string/builder.h>

namespace NAlice {

namespace {

struct TParseFlags {
    // requested the current time (i.e. the current weather; not today weather)
    bool CurrentTimeRequest = false;
    // means that there is at least one user entered absolute value in date (not in time)
    // it is needed for calculating in ranges
    // i.e. { days: 1, months: 2, months_relative: true } since days is absolute
    bool HasCustomAbs = false;
};

struct TDayPart {
    ui8 RBound;
    TDateTime::EDayPart Part;
    TStringBuf Name;
};

const TDayPart SORTED_DAY_PARTS[] = {
    { 6, TDateTime::EDayPart::Night, TStringBuf("night") },
    { 12, TDateTime::EDayPart::Morning, TStringBuf("morning") },
    { 18, TDateTime::EDayPart::Day, TStringBuf("day") },
    { 23, TDateTime::EDayPart::Evening, TStringBuf("evening") },
    { 24, TDateTime::EDayPart::Night, TStringBuf("night") },
    // duplicate 24 is not a mistake!!!
    { 24, TDateTime::EDayPart::WholeDay, TStringBuf("whole_day") },
};

struct TAdjustAmount {
    TAdjustAmount(TDateTime::TSplitTime::EField f)
        : TAdjustAmount(f, 1)
    {
    }
    TAdjustAmount(TDateTime::TSplitTime::EField f, i32 amount)
        : Field(f)
        , Amount(amount)
    {
    }

    const TDateTime::TSplitTime::EField Field;
    const i32 Amount;
};

class TDatePart {
public:
    using TValueType = i64;
    struct TCustomValue {
        TValueType Value = 0;
        bool IsRelative = false;
    };

    /** This class choose to get value between user (for the given DatePart) and minimum value
     * (ie: mon/day - 1, hour/min/sec - 0)
     * queries: [2017] means 2017 Jan 01, 00:00:00 but not 2017 userCurrentDateTime
     * or [Sep] means currentUserYear !Sep! 1 00:00:00, etc...
     */
    class TChooseValue {
    public:
        TChooseValue* operator()(TValueType userValue, TValueType minValue) {
            CurrentUserValue = userValue;
            CurrentMinValue = minValue;
            return this;
        }

        TChooseValue* operator()(TValueType userValue) {
            return operator()(userValue, userValue);
        }

        TValueType Get(bool hasCustomWithoutAdjust) {
            Y_ASSERT(CurrentUserValue);
            Y_ASSERT(CurrentMinValue);
            TValueType rval = NeedMinValue ? *CurrentMinValue : *CurrentUserValue;
            if (hasCustomWithoutAdjust) {
                NeedMinValue = true;
            }
            return rval;
        }

        void FlagNeedMinValue() {
            NeedMinValue = true;
        }

    private:
        bool NeedMinValue = false;
        TMaybe<TValueType> CurrentUserValue;
        TMaybe<TValueType> CurrentMinValue;
    };

    TDatePart(TAdjustAmount adjustAmount)
        : Absolute(0)
        , AdjustAmount(adjustAmount)
        , IsForNowValid(false)
    {
    }

    TDatePart(bool* hasCustomAbs, const NSc::TValue& value, bool relative, TAdjustAmount adjustAmount, TChooseValue* userValue)
        : TDatePart(adjustAmount)
    {
        UpdateValue(hasCustomAbs, value, relative, userValue, false);
    }

    void Adjust(TDateTime::TSplitTime* dst, size_t* isNowRequestedCounter) const {
        if (AdjustValue) {
            dst->Add(AdjustAmount.Field, AdjustAmount.Amount * *AdjustValue);
        }

        if (IsForNowValid) {
            ++*isNowRequestedCounter;
        }
    }

    void AddRelative(i64 value) {
        if (AdjustValue) {
            *AdjustValue += value;
        }
        else {
            AdjustValue = value;
        }
    }

    TString ToString() const {
        TStringBuilder s;
        s << "[custom:" << Custom << "][isNowValid:" << IsForNowValid << "][abs:" << Absolute << "] ";
        if (AdjustValue) {
            s << "Adjust (" << *AdjustValue << ")";
        }
        else {
            s << "NoAdjust";
        }

        return s;
    }

    void UpdateValue(bool* hasCustomAbs, TValueType userInput, bool relative, TChooseValue* userCurrent, bool isForNow = false) {
        Custom.ConstructInPlace(TCustomValue{userInput, relative});
        IsForNowValid = isForNow && relative && userInput == 0;
        if (relative) {
            AdjustValue = userInput;
            Absolute = userCurrent->Get(!AdjustValue);
        }
        else {
            if (hasCustomAbs) {
                *hasCustomAbs = true;
            }
            Absolute = userInput;
            userCurrent->FlagNeedMinValue();
        }
    }

    void UpdateValue(const NSc::TValue& value, bool relative, TChooseValue* userCurrent, bool isForNow = false) {
        UpdateValue(nullptr, value, relative, userCurrent, isForNow);
    }

    void UpdateValue(bool* hasCustomAbs, const NSc::TValue& value, bool relative, TChooseValue* userCurrent, bool isForNow = false) {
        if (!value.IsNull()) {
            UpdateValue(hasCustomAbs, value.ForceIntNumber(0), relative, userCurrent, isForNow);
        }
        else {
            Custom.Clear();
            Absolute = userCurrent->Get(false);
            IsForNowValid = true;
        }
    }

    // this field is used only to set up an initial date for TSplitTime
    ui64 Absolute;
    TMaybe<TValueType> AdjustValue;
    TMaybe<TCustomValue> Custom;

private:
    const TAdjustAmount AdjustAmount;
    bool IsForNowValid;
};

void WeekdayAdjust(TDateTime::TSplitTime* dst, i64 value, bool lookForward) {
    i8 adjustValue = value - (dst->WDay() > 0 ? dst->WDay() : 7);
    if (adjustValue < 0 && lookForward) {
        adjustValue += 7;
    }

    if (adjustValue != 0) {
        dst->Add(TDateTime::TSplitTime::EField::F_DAY, adjustValue);
    }
}


class TDatePartWeeks {
public:
    TDatePartWeeks(const NSc::TValue& value, bool isDateRelative, bool* hasCustomAbs)
        : Relative(isDateRelative || value["weeks_relative"].GetBool(false))
        , Value(!value["weeks"].IsNull() ? value["weeks"].ForceIntNumber(0) : TMaybe<i64>())
    {
        if (!*hasCustomAbs) {
            *hasCustomAbs = !Relative && Value.Defined();
        }
    }

    void Adjust(TDateTime::TSplitTime* dst, bool isRange, const TDateTime::TSplitTime* leftBound, size_t* isNowRequestedCounter) const {
        if (!Value) {
            ++*isNowRequestedCounter;
            return;
        }

        if (!*Value) {
            return;
        }

        i64 v = *Value * 7;

        if (isRange) {
            if (Relative) {
                // { start: { weeks: 0, weeks_relative: true }, end: { weeks: 1, weeks_relative: true } }
                dst->Add(TDateTime::TSplitTime::EField::F_DAY, v);

                // this is needed for adjusting queries which asks about next and later weeks
                // { start: { weeks: 1, ... }, end: { weeks: 2, ...}
                // the behaviour is the following:
                // * current week: get current days + 7 days
                // * next and later: get next/X week from mon to sun
                // PS: if leftBound is not null it means "end" otherwise "start"
                if (!leftBound || *Value != 1) {
                    // some magic since Sun is 0 and Mon is 1
                    // 1 is monday
                    if (1 != dst->WDay()) {
                        i8 adjustDOW = dst->WDay() > 1 ? (1 - dst->WDay()) : -6;
                        dst->Add(TDateTime::TSplitTime::EField::F_DAY, adjustDOW);
                    }
                }
            }
            else {
                // TODO implement absoulte values for weeks (but it is strange query)
                // { start: { weeks: 0 }, end: { weeks: 5 } }
                // погода с первой по пятую недели!!!
            }
        }
        else if (Relative) {
            dst->Add(TDateTime::TSplitTime::EField::F_DAY, v);
        }
        /* TODO implement absolute value
         * { weeks: 0 }
         * погода на первую неделю!!!
        else {
        }
        */
    }

    bool HasCustom() const {
        return Value.Defined();
    }

    bool IsRelative() const {
        return Relative;
    }

private:
    const bool Relative;
    const TMaybe<i64> Value;
};

class TDatePartWeekday {
public:
    TDatePartWeekday(const NSc::TValue& dow, const TDatePartWeeks& weeks, bool* hasCustomAbs, const TDateTime::TSplitTime** initialTime, const TDateTime::TSplitTime* leftBound)
        : Value(!dow.IsNull() ? TMaybe<i8>(dow) : TMaybe<i8>())
        , Weeks(weeks)
    {
        if (!*hasCustomAbs) {
            *hasCustomAbs = Value.Defined();
        }

        if (Value && leftBound) {
            *initialTime = leftBound;
        }
    }

    void Adjust(TDateTime::TSplitTime* dst, bool lookForward, size_t* isNowRequestedCounter) const {
        if (!Value) {
            ++*isNowRequestedCounter;
            return;
        }

        // if leftBound is not null it means that it is range and the "end" is in place!
        // in this case we want to move DOW forward anyway (no matter lookForward)
        // ie: { start: { weekday: 5 }, end: { weekday: 4 } },
        // "end" is Thu must be in the next week than "start"
        WeekdayAdjust(dst, *Value, !Weeks.HasCustom() && lookForward);

        // clear flag that the date is for current! FIXME
        //dayHasSpecified = true;
    }

    bool HasCustom() const {
        return Value.Defined();
    }

private:
    const TMaybe<i8> Value;
    const TDatePartWeeks& Weeks;
};

class TDatePartTime {
public:
    TDatePartTime(const NSc::TValue& v, const TDateTime::EDayPart* dayPart, const TDateTime::TSplitTime& ut, TDatePart::TChooseValue& userValue)
        : HoursValue(TDateTime::TSplitTime::EField::F_HOUR)
        , MinutesValue(TDateTime::TSplitTime::EField::F_MIN)
        , SecondsValue(TDateTime::TSplitTime::EField::F_SEC)
    {
        if (!dayPart || *dayPart == TDateTime::EDayPart::WholeDay) {
            bool isTimeRelative = v["time_relative"].GetBool(false);
            if (!isTimeRelative) {
                isTimeRelative = v["hours_relative"].GetBool(false)
                    || v["minutes_relative"].GetBool(false)
                    || v["seconds_relative"].GetBool(false);
            }

            HoursValue.UpdateValue(v["hours"], isTimeRelative, userValue(ut.Hour(), 0));
            MinutesValue.UpdateValue(v["minutes"], isTimeRelative, userValue(ut.Min(), 0));
            SecondsValue.UpdateValue(v["seconds"], isTimeRelative, userValue(ut.Sec(), 0), true);
        }
        else {
            ui8 idx = static_cast<ui8>(*dayPart);
            HoursValue.UpdateValue(idx ? SORTED_DAY_PARTS[idx - 1].RBound : 0, false, userValue(ut.Hour(), 0));
            MinutesValue.UpdateValue(0, false, userValue(ut.Min(), 0));
            SecondsValue.UpdateValue(0, false, userValue(ut.Sec(), 0), true);
        }

        HasCustom = HoursValue.Custom.Defined() || MinutesValue.Custom.Defined() || SecondsValue.Custom.Defined();
    }

    void Adjust(TDateTime::TSplitTime* dst, size_t* isNowRequestedCounter) const {
        HoursValue.Adjust(dst, isNowRequestedCounter);
        MinutesValue.Adjust(dst, isNowRequestedCounter);
        SecondsValue.Adjust(dst, isNowRequestedCounter);
    }

    i8 Hours() const {
        return HoursValue.Absolute;
    }

    i8 Minutes() const {
        return MinutesValue.Absolute;
    }

    i8 Seconds() const {
        return SecondsValue.Absolute;
    }

    bool HasCustomValue() const {
        return HasCustom;
    }

private:
    bool HasCustom = false;
    TDatePart HoursValue;
    TDatePart MinutesValue;
    TDatePart SecondsValue;
};

} // anon namespace

TDateTime::TSplitTime::TSplitTime(const NDatetime::TTimeZone& tz, time_t epoch)
    : Time(NDatetime::Convert(TInstant::Seconds(epoch), tz))
    , TZ(tz)
{
}

TDateTime::TSplitTime::TSplitTime(const NDatetime::TTimeZone& tz, ui32 year, ui32 mon, ui32 day, ui32 h, ui32 m, ui32 s)
    : Time(NDatetime::TCivilSecond(year, mon, day, h, m, s))
    , TZ(tz)
{
}

TDateTime::TSplitTime& TDateTime::TSplitTime::Add(TDateTime::TSplitTime::EField f, i32 amount) {
    if (!amount)
        return *this;
    using namespace NDatetime;
    if (f == EField::F_YEAR) {
        Time = AddYears(Time, amount);
    } else if (f == EField::F_MON) {
        Time = AddMonths(Time, amount);
    } else if (f == EField::F_DAY) {
        Time = AddDays(Time, amount);
    } else if (f == EField::F_HOUR) {
        Time = AddHours(Time, amount);
    } else if (f == EField::F_MIN) {
        Time = AddMinutes(Time, amount);
    } else if (f == EField::F_SEC) {
        Time = AddSeconds(Time, amount);
    }

    return *this;
}

//FIXME: remove AsTimeT()!!!
ui64 TDateTime::TSplitTime::AsTimeT() const {
    if (Time.year() < 1970) {
        return 0;
    }
    return NDatetime::Convert(Time, TZ).Seconds();
}

TDateTime::TDateTime(const TSplitTime& st)
    : CurrentDayPart(TimeToDayPart(st))
    , ST(st)
{
}

TDateTime::TDateTime(const TSplitTime& st, TStringBuf dayPart)
    : TDateTime(st, StringToDayPart(dayPart))
{
}

TDateTime::TDateTime(const TSplitTime& st, EDayPart dayPart)
    : CurrentDayPart(dayPart)
    , ST(st)
{
}

TStringBuf TDateTime::DayPartAsString() const {
    return DayPartAsString(CurrentDayPart);
}

// static
TStringBuf TDateTime::DayPartAsString(EDayPart dp) {
    for (const TDayPart& idp : SORTED_DAY_PARTS) {
        if (dp == idp.Part) {
            return idp.Name;
        }
    }
    return TStringBuf("invalid");
}

// static
TDateTime::EDayPart TDateTime::StringToDayPart(TStringBuf dp) {
    for (const TDayPart& idp : SORTED_DAY_PARTS) {
        if (idp.Name == dp) {
            return idp.Part;
        }
    }
    return TDateTime::EDayPart::Invalid;
}

ssize_t TDateTime::OffsetWidth(const TDateTime::TSplitTime& ut) const {
    using namespace NDatetime;
    TCivilDay day = TCivilDay(ST.RealYear(), ST.RealMonth(), ST.MDay());
    TCivilDay utDay = TCivilDay(ut.RealYear(), ut.RealMonth(), ut.MDay());
    return (day - utDay);
}

bool operator >(TDateTime::EDayPart lhs, TDateTime::EDayPart rhs) {
    return static_cast<size_t>(lhs) > static_cast<size_t>(rhs);
}

bool operator <(TDateTime::EDayPart lhs, TDateTime::EDayPart rhs) {
    return static_cast<size_t>(lhs) < static_cast<size_t>(rhs);
}

// static
TDateTime::EDayPart TDateTime::TimeToDayPart(const TDateTime::TSplitTime& st) {
    for (const auto& part : SORTED_DAY_PARTS) {
        if (st.Hour() < part.RBound) {
            return part.Part;
        }
    }
    return TDateTime::EDayPart::Invalid;
}


TDateTimeList::TDateTimeList() = default;

TDateTimeList::TDateTimeList(const TDateTime& dt) {
    Days.push_back(dt);
}

TDateTime TDaysParser::ParseOneDay(const NSc::TValue& v,
                                   const TDateTime& ut,
                                   const TDateTime::EDayPart* dayPartFromSlot,
                                   const TDateTime::TSplitTime* leftBound)
{
    HasCustomAbs = false;
    CurrentTimeRequest = false;

    using namespace NDatetime;
    using EF = TDateTime::TSplitTime::EField;

    const TDateTime::TSplitTime* initialTime = &ut.SplitTime();

    TDatePart::TChooseValue userValue;

    bool isDateRelative = v["date_relative"].GetBool(false);
    bool isYearRelative = v["years_relative"].GetBool(false);

    TDatePartWeeks weeks(v, isDateRelative, &HasCustomAbs);
    TDatePartWeekday weekday(v["weekday"], weeks, &HasCustomAbs, &initialTime, leftBound);

    NSc::TValue fullYear;

    if (!v["years"].IsNull()) {
        if (!isDateRelative && !isYearRelative) {
            int yearsNum = v["years"].ForceIntNumber(-1);
            if (yearsNum >= 0 && yearsNum < 100) {
                if (yearsNum <= static_cast<int>(initialTime->RealYear() - 1990)) { // (2017 - 2000 + 10) add 10 year to current year for requests about future
                    yearsNum += 2000;
                } else {
                    yearsNum += 1900;
                }
            }
            if (yearsNum > 0) {
                fullYear.SetIntNumber(yearsNum);
            }
        } else {
            fullYear.SetIntNumber(v["years"].ForceIntNumber(0));
        }
    }

    TDatePart years(&HasCustomAbs, fullYear, isDateRelative || isYearRelative, EF::F_YEAR, userValue(initialTime->RealYear()));
    TDatePart months(&HasCustomAbs, v["months"], isDateRelative || v["months_relative"].GetBool(false), EF::F_MON, userValue(initialTime->RealMonth(), 1));
    TDatePart days(&HasCustomAbs, v["days"], isDateRelative || v["days_relative"].GetBool(false), EF::F_DAY, userValue(initialTime->MDay(), 1));
    TDatePartTime time(v, dayPartFromSlot, *initialTime, userValue);

    TDateTime::TSplitTime adjusted(initialTime->TimeZone(),
        years.Absolute, months.Absolute, days.Absolute,
        time.Hours(), time.Minutes(), time.Seconds()
    );

    // also check if it is date for now
    size_t isNowRequestedCounter = 0;

    years.Adjust(&adjusted, &isNowRequestedCounter);
    months.Adjust(&adjusted, &isNowRequestedCounter);
    weeks.Adjust(&adjusted, IsRange, leftBound, &isNowRequestedCounter);
    days.Adjust(&adjusted, &isNowRequestedCounter);
    weekday.Adjust(&adjusted, Params().LookForward, &isNowRequestedCounter);
    time.Adjust(&adjusted, &isNowRequestedCounter);

    const bool dayHasSpecified = days.Custom.Defined();

    if (isNowRequestedCounter && dayPartFromSlot) {
        isNowRequestedCounter = 0;
    }

    if (isNowRequestedCounter == 8) {
        CurrentTimeRequest = true;
    }
    else if (Params().LookForward && !dayHasSpecified && adjusted.ToString("%F") == initialTime->ToString("%F")) {
        // логика следующая:
        //   если текущий день
        //    и день не был указан (но все равно получается что сегодня)
        //    и время суток (либо из слота, либо из времени) меньше текущего
        //   тогда добавляем день, это нужно для запросов вида:
        //    (сейчас 19:00) [погода утром] - это уже утро следующего лня, при этом [погода сегодня утром]
        //    не должна прибавлять день
        TMaybe<TDateTime::EDayPart> dayPartForAdjust;
        if (dayPartFromSlot && *dayPartFromSlot != TDateTime::EDayPart::WholeDay) {
            for (const TDayPart& idp : SORTED_DAY_PARTS) {
                if (idp.Part == *dayPartFromSlot) {
                    dayPartForAdjust = idp.Part;
                    break;
                }
            }
        }
        else {
            if (adjusted.Hour() != 23 ) {
                dayPartForAdjust = TDateTime::TimeToDayPart(adjusted);
            }
        }

        if (dayPartForAdjust && (ut.DayPart() > *dayPartForAdjust) || (initialTime->Hour() == 23)) {
            adjusted.Add(EF::F_DAY, 1);
        }
    }

    // XXX (@petrk) This is "kostyl" that moves date to month or year forward if date is in the past.
    if (Params().LookForward && !years.Custom.Defined() && days.Custom.Defined() && days.Custom->Value > 0) {
        constexpr i64 daySeconds = -TDuration::Days(1).Seconds();
        const i64 diffSeconds = adjusted.AsTimeT() - ut.SplitTime().AsTimeT();
        if (diffSeconds < daySeconds) {
            if (!months.Custom.Defined()) {
                adjusted.Add(TDateTime::TSplitTime::EField::F_MON, 1);
                adjusted.SetDay(days.Absolute);
            }
            else {
                adjusted.Add(TDateTime::TSplitTime::EField::F_YEAR, 1);
                adjusted.SetDay(days.Absolute);
            }
        }
    }

    return dayPartFromSlot
        ? TDateTime(adjusted, *dayPartFromSlot)
        : (time.HasCustomValue()
           ? TDateTime(adjusted)
           : TDateTime(adjusted, TDateTime::EDayPart::WholeDay));
}

TDateTime TDaysParser::ParseOneDay(const TString& v,
                                   const TDateTime& ut,
                                   const TDateTime::EDayPart* dayPartFromSlot,
                                   const TDateTime::TSplitTime* leftBound)
{
    return ParseOneDay(NSc::TValue::FromJson(v), ut, dayPartFromSlot, leftBound);
}

} // namespace NAlice

template <>
void Out<NAlice::TDateTime>(IOutputStream& out, const NAlice::TDateTime& dt) {
    out << dt.SplitTime().ToString() << '[' << dt.DayPartAsString() << ']';
}

template <>
void Out<NAlice::TDateTime::EDayPart>(IOutputStream& out, NAlice::TDateTime::EDayPart dayPart) {
    out << NAlice::TDateTime::DayPartAsString(dayPart);
}

template <>
void Out<NAlice::TDateTimeList>(IOutputStream& out, const NAlice::TDateTimeList& src) {
    if (src.IsNow()) {
        out << "TDateTimeList: now" << Endl;
    }
    else {
        for (const NAlice::TDateTime& dt : src) {
            out << "TDateTimeList: day: " << dt << Endl;
        }
    }
}

template <>
void Out<NAlice::TDatePart::TCustomValue>(IOutputStream& out, const NAlice::TDatePart::TCustomValue& src) {
    out << "relative: " << src.IsRelative << ", value: " << src.Value;
}
