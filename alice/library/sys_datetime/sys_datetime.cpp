///
/// Parser for sys.datetime, sys.date, sys.time objects
///

#include "sys_datetime.h"
#include "util/datetime/base.h"

#include <alice/protos/data/entities/datetime.pb.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/timezone_conversion/civil.h>

#include <util/datetime/systime.h>
#include <util/generic/maybe.h>

namespace NAlice {

namespace {

// Internal structure to analyze all JSON fields in a loop
struct TJsonNames {
    const TStringBuf FirstString; // Main json string
    const TStringBuf AdditionalString; // Optional JSON string, used with RELATIVE_RULES only
    TSysDatetimeParser::TParseDateTime::TValue TSysDatetimeParser::TParseDateTime::*Ptr; // Pointer to TParseDateTime::TValue field
};

// Rules for relative JSON keys
const TVector<TJsonNames> RELATIVE_RULES = {
    {"seconds_relative", "seconds", &TSysDatetimeParser::TParseDateTime::Seconds},
    {"minutes_relative", "minutes", &TSysDatetimeParser::TParseDateTime::Minutes},
    {"hours_relative", "hours", &TSysDatetimeParser::TParseDateTime::Hour},
    {"days_relative", "days", &TSysDatetimeParser::TParseDateTime::Day},
    {"months_relative", "months", &TSysDatetimeParser::TParseDateTime::Month},
    {"years_relative", "years", &TSysDatetimeParser::TParseDateTime::Year},
    {"weeks_relative", "weeks", &TSysDatetimeParser::TParseDateTime::WeeksCount}
};

// Rules for absolute JSON keys
const TVector<TJsonNames> ABSOLUTE_RULES = {
    {"years", "", &TSysDatetimeParser::TParseDateTime::Year},
    {"months", "", &TSysDatetimeParser::TParseDateTime::Month},
    {"days", "", &TSysDatetimeParser::TParseDateTime::Day},
    {"hours", "", &TSysDatetimeParser::TParseDateTime::Hour},
    {"minutes", "", &TSysDatetimeParser::TParseDateTime::Minutes},
    {"seconds", "", &TSysDatetimeParser::TParseDateTime::Seconds},
    {"weekday", "", &TSysDatetimeParser::TParseDateTime::DayOfWeek},
    {"period", "", nullptr}
};

// All standard fields
const TVector<TSysDatetimeParser::TParseDateTime::TValue TSysDatetimeParser::TParseDateTime::*> FIELD_REFERENCES = {
    &TSysDatetimeParser::TParseDateTime::Year,
    &TSysDatetimeParser::TParseDateTime::Month,
    &TSysDatetimeParser::TParseDateTime::Day,
    &TSysDatetimeParser::TParseDateTime::Hour,
    &TSysDatetimeParser::TParseDateTime::Minutes,
    &TSysDatetimeParser::TParseDateTime::Seconds,
    &TSysDatetimeParser::TParseDateTime::DayOfWeek,
    &TSysDatetimeParser::TParseDateTime::WeeksCount
};

} // anonimous namespace

//
// Merge two dateties together
// @return: false if both dates contains the same fields filled
//
bool TSysDatetimeParser::TParseDateTime::Merge(const TSysDatetimeParser::TParseDateTime& rhs) {
    // Preliminary check
    for (const auto it : FIELD_REFERENCES) {
        if (At(it).IsFilled() && rhs.At(it).IsFilled()) {
            return false;
        }
    }

    // Move fields
    for (const auto it : FIELD_REFERENCES) {
        if (At(it).IsNotFilled() && rhs.At(it).IsFilled()) {
            // Move to first slot
            At(it) = rhs.At(it);
        }
    }
    return true;
}

///
/// Parse a source JSON and try to full raw data stuct
///
TMaybe<TSysDatetimeParser> TSysDatetimeParser::ParseJson(const NJson::TJsonValue& slotValue) {
    TSysDatetimeParser result;
    return result.ParseInternal(slotValue) ? TMaybe<TSysDatetimeParser>(result) : Nothing();
}

TMaybe<TSysDatetimeParser> TSysDatetimeParser::Parse(const TString& slotValue) {
    NJson::TJsonValue jsonData;
    if (!NJson::ReadJsonTree(slotValue, &jsonData, /* throwOnError = */ false)) {
        // Can't parse
        return Nothing();
    }
    TSysDatetimeParser result;
    return result.ParseInternal(jsonData) ? TMaybe<TSysDatetimeParser>(result) : Nothing();
}

//
// Parse from PROTO
//
TMaybe<TSysDatetimeParser> TSysDatetimeParser::Parse(const TSysDatetimeValue& proto) {
    TSysDatetimeParser result;
    bool rc1 = false;
    bool rc2 = false;
    if (proto.HasDateValue()) {
        rc1 = result.ParseInternal(proto.GetDateValue());
    }
    if (proto.HasTimeValue()) {
        rc2 = result.ParseInternal(proto.GetTimeValue());
    }
    return (rc1 || rc2) ? TMaybe<TSysDatetimeParser>(result) : Nothing();
}

TMaybe<TSysDatetimeParser> TSysDatetimeParser::Parse(const TSysTimeValue& proto) {
    TSysDatetimeParser result;
    return result.ParseInternal(proto) ? TMaybe<TSysDatetimeParser>(result) : Nothing();
}

TMaybe<TSysDatetimeParser> TSysDatetimeParser::Parse(const TSysDateValue& proto) {
    TSysDatetimeParser result;
    return result.ParseInternal(proto) ? TMaybe<TSysDatetimeParser>(result) : Nothing();
}

TMaybe<TSysDatetimeParser> TSysDatetimeParser::Today() {
    TSysDatetimeParser result;
    result.GetRawDatetime().Day.SetRelative(0);
    return TMaybe<TSysDatetimeParser>(result);
}
TMaybe<TSysDatetimeParser> TSysDatetimeParser::Yesterday() {
    TSysDatetimeParser result;
    result.GetRawDatetime().Day.SetRelative(-1);
    return TMaybe<TSysDatetimeParser>(result);
}
TMaybe<TSysDatetimeParser> TSysDatetimeParser::Tomorrow() {
    TSysDatetimeParser result;
    result.GetRawDatetime().Day.SetRelative(1);
    return TMaybe<TSysDatetimeParser>(result);
}


//
// Get as a JSON
//
NJson::TJsonValue TSysDatetimeParser::GetAsJsonDatetime() const {
    NJson::TJsonValue root;

    for (const auto& it : RELATIVE_RULES) {
        if (ParsedDatetime_.At(it.Ptr).IsRelative()) {
            root[it.FirstString] = true;
            root[it.AdditionalString] = ParsedDatetime_.At(it.Ptr).Get();
        } else if (ParsedDatetime_.At(it.Ptr).IsAbsolute()) {
            root[it.AdditionalString] = ParsedDatetime_.At(it.Ptr).Get();
        }
    }

    // Additional values 
    if (ParsedDatetime_.DayOfWeek.IsFilled()) {
        root["weekday"] = ParsedDatetime_.DayOfWeek.Get();
    }
    switch (ParsedDatetime_.Period) {
        case TParseDateTime::PeriodNone:
            break;
        case TParseDateTime::PeriodAM:
            root["period"] = "am";
            break;
        case TParseDateTime::PeriodPM:
            root["period"] = "pm";
            break;
    }
    return std::move(root);
}
//
// Get as a PROTO
//
TSysDatetimeValue TSysDatetimeParser::GetAsProtoDatetime() const {
    TSysDatetimeValue proto = {};
    *proto.MutableDateValue() = GetAsProtoDate();
    *proto.MutableTimeValue() = GetAsProtoTime();
    return proto;
}

TSysTimeValue TSysDatetimeParser::GetAsProtoTime() const {
    TSysTimeValue proto = {};
    
    if (ParsedDatetime_.Hour.IsFilled()) {
        proto.SetHours(ParsedDatetime_.Hour.Get());
        if (ParsedDatetime_.Hour.IsRelative()) {
            proto.SetHoursRelative(true);
        }
    }
    if (ParsedDatetime_.Minutes.IsFilled()) {
        proto.SetMinutes(ParsedDatetime_.Minutes.Get());
        if (ParsedDatetime_.Minutes.IsRelative()) {
            proto.SetMinutesRelative(true);
        }
    }
    if (ParsedDatetime_.Seconds.IsFilled()) {
        proto.SetSeconds(ParsedDatetime_.Seconds.Get());
        if (ParsedDatetime_.Seconds.IsRelative()) {
            proto.SetSecondsRelative(true);
        }
    }
    switch (ParsedDatetime_.Period) {
        case TParseDateTime::PeriodNone:
            // nothing to set
            break;
        case TParseDateTime::PeriodAM:
            proto.SetPeriod("am");
            break;
        case TParseDateTime::PeriodPM:
            proto.SetPeriod("pm");
            break;
    }
    return proto;
}

TSysDateValue TSysDatetimeParser::GetAsProtoDate() const {
    TSysDateValue proto = {};

    if (ParsedDatetime_.Year.IsFilled()) {
        proto.SetYears(ParsedDatetime_.Year.Get());
        if (ParsedDatetime_.Year.IsRelative()) {
            proto.SetYearsRelative(true);
        }
    }
    if (ParsedDatetime_.Month.IsFilled()) {
        proto.SetMonths(ParsedDatetime_.Month.Get());
        if (ParsedDatetime_.Month.IsRelative()) {
            proto.SetMonthsRelative(true);
        }
    }
    if (ParsedDatetime_.Day.IsFilled()) {
        proto.SetDays(ParsedDatetime_.Day.Get());
        if (ParsedDatetime_.Day.IsRelative()) {
            proto.SetDaysRelative(true);
        }
    }
    if (ParsedDatetime_.WeeksCount.IsFilled()) {
        proto.SetWeeksCount(ParsedDatetime_.WeeksCount.Get());
        if (ParsedDatetime_.WeeksCount.IsRelative()) {
            proto.SetWeeksRelative(true);
        }
    }
    if (ParsedDatetime_.DayOfWeek.IsFilled()) {
        proto.SetDayOfWeek(ParsedDatetime_.DayOfWeek.Get());
    }
    return proto;
}


///
/// Check parsing results
///
TSysDatetimeParser::EDatetimeParser TSysDatetimeParser::GetParseInfo() const {
    EDatetimeParser res = ParsedDatetime_.DayOfWeek.IsAbsolute() ? EDatetimeParser::FixedDayOfWeek : EDatetimeParser::Unknown;

    for (const auto it : FIELD_REFERENCES) {

        switch (ParsedDatetime_.At(it).GetValueInfo()) {
            case TParseDateTime::TValue::ValueInfoNotFilled:
                break;
            case TParseDateTime::TValue::ValueInfoAbsolute:
                if (it == &TSysDatetimeParser::TParseDateTime::DayOfWeek && res == EDatetimeParser::FixedDayOfWeek) {
                    break;
                }
                if (res == EDatetimeParser::Unknown || res == EDatetimeParser::Fixed) {
                    res = EDatetimeParser::Fixed;
                } else {
                    res = EDatetimeParser::Mix;
                }
                break;
            case TParseDateTime::TValue::ValueInfoRelative:
                if (ParsedDatetime_.At(it).Get() >= 0) {
                    if (res == EDatetimeParser::Unknown || res == EDatetimeParser::RelativeFuture) {
                        res = EDatetimeParser::RelativeFuture;
                    } else if (res == EDatetimeParser::RelativePast) {
                        res = EDatetimeParser::RelativeMix;
                    } else {
                        res = EDatetimeParser::Mix;
                    }
                } else {
                    if (res == EDatetimeParser::Unknown || res == EDatetimeParser::RelativePast) {
                        res = EDatetimeParser::RelativePast;
                    } else if (res == EDatetimeParser::RelativeFuture) {
                        res = EDatetimeParser::RelativeMix;
                    } else {
                        res = EDatetimeParser::Mix;
                    }
                }
                break;
        }
    }
    return res;
}

//
// Alalyze data: does struct contains only date, only time or both fields
//
TSysDatetimeParser::EDatetimeContent TSysDatetimeParser::GetParseContent() const {
    bool dateFieldsPresent = ParsedDatetime_.Year.IsFilled() || 
                             ParsedDatetime_.Month.IsFilled() || 
                             ParsedDatetime_.Day.IsFilled() || 
                             ParsedDatetime_.WeeksCount.IsFilled() || 
                             ParsedDatetime_.DayOfWeek.IsFilled();
    bool timeFieldsPresent = ParsedDatetime_.Hour.IsFilled() ||
                             ParsedDatetime_.Minutes.IsFilled() ||
                             ParsedDatetime_.Seconds.IsFilled() ||
                             ParsedDatetime_.Period != TParseDateTime::PeriodNone;

    if (dateFieldsPresent && timeFieldsPresent) {
        return EDatetimeContent::DateTime;
    }
    if (dateFieldsPresent) {
        return EDatetimeContent::DateOnly;
    }
    if (timeFieldsPresent) {
        return EDatetimeContent::TimeOnly;
    }
    return EDatetimeContent::Unknown;
}

///
/// Generte final date and time using json instructions
/// @param date (IN/OUT) source date (YYYYMMDD)
/// @param tense - additional instruction to move relative dates to future or past tense
///
NDatetime::TCivilSecond TSysDatetimeParser::GetTargetDateTime(const NDatetime::TCivilSecond& dateCurrent, ETense tense) const {
    NDatetime::TCivilSecond dateResult = dateCurrent;
    TParseDateTime dtCopy = ParsedDatetime_; // Make a copy of ParsedDatetime_ for preliminary fixup

    // Fixup values in dtCopy
    if (dtCopy.Period == TParseDateTime::PeriodPM && dtCopy.Hour.IsAbsolute() && dtCopy.Hour.Get() <= 12) {
        dtCopy.Hour.SetAbsolute(dtCopy.Hour.Get() + 12);
    }
    if (dtCopy.Year.IsAbsolute() && dtCopy.Year.Get() < 100) {
        // TenseDefault: see comment for SetDefautYearCorrection function
        // TensePast / TenseFuture - set by current tm
        int maxRangedYear = DefaultYearCorrection_ + dateResult.year();
        int decidedYear = (dateResult.year() / 100) * 100 + dtCopy.Year.Get();

        switch (tense) {
            case TenseDefault:
                if (decidedYear < maxRangedYear) {
                    // Use as is
                    dtCopy.Year.SetAbsolute(decidedYear);
                } else {
                    // Use past
                    dtCopy.Year.SetAbsolute(decidedYear - 100);
                }
                break;
            case TensePast:
                dtCopy.Year.SetAbsolute(decidedYear > dateResult.year() ? decidedYear - 100 : decidedYear);
                break;
            case TenseFuture:
                dtCopy.Year.SetAbsolute(decidedYear < dateResult.year() ? decidedYear + 100 : decidedYear);
                break;
        }
    }

    switch (tense) {
        case TenseDefault:
            // No additional operations required
            break;
        case TenseFuture:
            for (auto it : FIELD_REFERENCES) {
                if (dtCopy.At(it).IsRelative() && dtCopy.At(it).Get() < 0) {
                    dtCopy.At(it).SetRelative(-dtCopy.At(it).Get());
                }
            }
            break;
        case TensePast:
            // Fixup relative dates
            for (auto it : FIELD_REFERENCES) {
                if (dtCopy.At(it).IsRelative() && dtCopy.At(it).Get() > 0) {
                    dtCopy.At(it).SetRelative(-dtCopy.At(it).Get());
                }
            }
            break;
    }

    //
    // Setup absolute values
    //
    if (dtCopy.Year.IsAbsolute() ||
        dtCopy.Month.IsAbsolute() ||
        dtCopy.Day.IsAbsolute() || 
        dtCopy.Hour.IsAbsolute() ||
        dtCopy.Minutes.IsAbsolute() ||
        dtCopy.Seconds.IsAbsolute()) {

        // Recreate date variables with new values
        int year = dtCopy.Year.IsAbsolute() ? dtCopy.Year.Get() : dateResult.year();
        int month = dtCopy.Month.IsAbsolute() ? dtCopy.Month.Get() : dateResult.month();
        int day = dtCopy.Day.IsAbsolute() ? dtCopy.Day.Get() : dateResult.day();
        int hour = dtCopy.Hour.IsAbsolute() ? dtCopy.Hour.Get() : dateResult.hour();
        int minute = dtCopy.Minutes.IsAbsolute() ? dtCopy.Minutes.Get() : dateResult.minute();
        int second = dtCopy.Seconds.IsAbsolute() ? dtCopy.Seconds.Get() : dateResult.second();

        dateResult = NDatetime::TCivilSecond(year, month, day, hour, minute, second);
    }


    // Adjust absolute dates using tense
    // This code handle special cases: 
    //      Day:5 (month not filled). Current day: 15, FUTURE -> move to next month
    //      Day:5 (month not filled). Current day: 15, PAST -> keep "as is"
    switch (tense) {
        case TenseDefault:
            // No additional operations required
            break;
        case TenseFuture:
            if (dateResult < dateCurrent) {
                if (dtCopy.Year.IsNotFilled() && dtCopy.Month.IsFilled()) {
                    dateResult = NDatetime::AddYears(dateResult, 1);
                } else if (dtCopy.Month.IsNotFilled() && dtCopy.Day.IsFilled()) {
                    dateResult = NDatetime::AddMonths(dateResult, 1);
                }
            }
            break;
        case TensePast:
            if (dateResult > dateCurrent) {
                if (dtCopy.Year.IsNotFilled() && dtCopy.Month.IsFilled()) {
                    dateResult = NDatetime::AddYears(dateResult, -1);
                } else if (dtCopy.Month.IsNotFilled() && dtCopy.Day.IsFilled()) {
                    dateResult = NDatetime::AddMonths(dateResult, -1);
                }
            }
            break;
    }

    //
    // Use 'human-hours' correction
    // See SetHumanCorrection() definition for more info
    //
    if (dtCopy.Day.IsRelative() && dtCopy.Day.Get() > 0) {
        const TDuration timeAfterMidnight = TDuration::Hours(dateCurrent.hour()) + TDuration::Minutes(dateCurrent.minute());
        if (timeAfterMidnight < HumanLagAfterMidnight_) {
            // Need to shift the current date to one less
            dtCopy.Day.SetRelative(dtCopy.Day.Get() - 1);
        }
    }

    //
    // Setup relative values
    //
    if (dtCopy.Year.IsRelative()) {
        dateResult = NDatetime::AddYears(dateResult, dtCopy.Year.Get());
    }
    if (dtCopy.Month.IsRelative()) {
        dateResult = NDatetime::AddMonths(dateResult, dtCopy.Month.Get());
    }
    if (dtCopy.Day.IsRelative()) {
        dateResult = NDatetime::AddDays(dateResult, dtCopy.Day.Get());
    }

    if (dtCopy.Hour.IsRelative()) {
        dateResult = NDatetime::AddHours(dateResult, dtCopy.Hour.Get());
    }
    if (dtCopy.Minutes.IsRelative()) {
        dateResult = NDatetime::AddMinutes(dateResult, dtCopy.Minutes.Get());
    }
    if (dtCopy.Seconds.IsRelative()) {
        dateResult = NDatetime::AddSeconds(dateResult, dtCopy.Seconds.Get());
    }

    // Known issue: phrase "вторник" -> {"weekday":2}
    //              phrase "следующий вторник" -> {"weekday":2,"weeks":1,"weeks_relative":true}
    // In case if we have both WeeksCount and DayOfWeek, we need to handle this case (see below)
    if (dtCopy.WeeksCount.IsRelative() && dtCopy.DayOfWeek.IsNotFilled()) {
        dateResult = NDatetime::AddDays(dateResult, 7 * dtCopy.WeeksCount.Get());
    }

    // Normalize date
    dateResult = NDatetime::TCivilSecond(dateResult.year(), dateResult.month(), dateResult.day(), dateResult.hour(), dateResult.minute(), dateResult.second());

    //
    // Fixup day of week
    //
    if (dtCopy.DayOfWeek.IsAbsolute()) {
        NDatetime::TCivilDay dateOnly(dateResult.year(), dateResult.month(), dateResult.day());
        NDatetime::TWeekday dayOfWeekFixed = static_cast<NDatetime::TWeekday>(dtCopy.DayOfWeek.Get()-1);

        switch (tense) {
            case TenseDefault:
                dateOnly = NDatetime::NearestWeekday(dateOnly, dayOfWeekFixed);
                break;
            case TenseFuture:
                if (dayOfWeekFixed == cctz::get_weekday(dateOnly)) {
                    // We asked about 'Friday' in 'Friday' - move to the next week
                    dateOnly = NDatetime::NextWeekday(dateOnly, dayOfWeekFixed);
                } else {
                    dateOnly = NDatetime::NearestWeekday(dateOnly, dayOfWeekFixed);
                }
                break;
            case TensePast:
                dateOnly = NDatetime::PrevWeekday(dateOnly, dayOfWeekFixed);
                break;
        }
        dateResult = NDatetime::TCivilSecond(dateOnly.year(), dateOnly.month(), dateOnly.day(), dateResult.hour(), dateResult.minute(), dateResult.second());

        if (dtCopy.WeeksCount.IsRelative()) {
            // Need to fixup date in case if we did not reach required week
            NDatetime::TCivilDay dateOnlyResult(dateResult.year(), dateResult.month(), dateResult.day());
            NDatetime::TCivilDay dateOnlyCurrent(dateCurrent.year(), dateCurrent.month(), dateCurrent.day());

            // Check for long distance jumps, we can fixup results for max 1 year jumps
            int weekDelta = 0;
            if (dateOnlyResult.year() == dateOnlyCurrent.year()) {
                weekDelta = NDatetime::GetYearWeek(dateOnlyResult) - NDatetime::GetYearWeek(dateOnlyCurrent);
            } else if (dateOnlyResult.year() == dateOnlyCurrent.year()+1) {
                weekDelta = NDatetime::GetYearWeek(dateOnlyResult, true) - NDatetime::GetYearWeek(dateOnlyCurrent);
            } else if (dateOnlyResult.year() == dateOnlyCurrent.year()-1) {
                weekDelta = NDatetime::GetYearWeek(dateOnlyResult) - NDatetime::GetYearWeek(dateOnlyCurrent, true);
            }
            dateResult = NDatetime::AddDays(dateResult, 7 * (dtCopy.WeeksCount.Get() - weekDelta));
        }
    }
    return  NDatetime::TCivilSecond(dateResult.year(), dateResult.month(), dateResult.day(), dateResult.hour(), dateResult.minute(), dateResult.second());
}

//
// Internal function
// Parse data from JSON string
//
bool TSysDatetimeParser::ParseInternal(const NJson::TJsonValue& jsonData) {
    bool bFoundSomeInstructions = false;
    // Try to locate all rules
    for (const auto& it : RELATIVE_RULES) {
        if (jsonData[it.FirstString].GetType() == NJson::JSON_BOOLEAN && jsonData[it.AdditionalString].GetType() == NJson::JSON_INTEGER) {
            ParsedDatetime_.At(it.Ptr).SetRelative(jsonData[it.AdditionalString].GetInteger());
            bFoundSomeInstructions = true;
        }
    }
    for (const auto& it : ABSOLUTE_RULES) {
        if (jsonData[it.FirstString].GetType() == NJson::JSON_INTEGER && ParsedDatetime_.At(it.Ptr).IsNotFilled()) {
            ParsedDatetime_.At(it.Ptr).SetAbsolute(jsonData[it.FirstString].GetInteger());
            bFoundSomeInstructions = true;
        }
    }

    // Additioally locating for am/pm data
    const NJson::TJsonValue period = jsonData["period"];
    if (period.GetType() == NJson::JSON_STRING) {
        bFoundSomeInstructions = true;
        if (period.GetString() == "pm") {
            ParsedDatetime_.Period = TParseDateTime::PeriodPM;
        } else if (period.GetString() == "am") {
            ParsedDatetime_.Period = TParseDateTime::PeriodAM;
        } else {
            return false;
        }
    }
    return bFoundSomeInstructions;
}

//
// Internal function
// Parse data from protobuf object
//
bool TSysDatetimeParser::ParseInternal(const TSysTimeValue& proto) {
    bool bFoundSomeInstructions = false;

    if (proto.HasHours()) {
        bFoundSomeInstructions = true;
        if (proto.HasHoursRelative() && proto.GetHoursRelative()) {
            ParsedDatetime_.Hour.SetRelative(proto.GetHours());
        } else {
            ParsedDatetime_.Hour.SetAbsolute(proto.GetHours());
        }
    }
    if (proto.HasMinutes()) {
        bFoundSomeInstructions = true;
        if (proto.HasMinutesRelative() && proto.GetMinutesRelative()) {
            ParsedDatetime_.Minutes.SetRelative(proto.GetMinutes());
        } else {
            ParsedDatetime_.Minutes.SetAbsolute(proto.GetMinutes());
        }
    }
    if (proto.HasSeconds()) {
        bFoundSomeInstructions = true;
        if (proto.HasSecondsRelative() && proto.GetSecondsRelative()) {
            ParsedDatetime_.Seconds.SetRelative(proto.GetSeconds());
        } else {
            ParsedDatetime_.Seconds.SetAbsolute(proto.GetSeconds());
        }
    }
    if (proto.HasPeriod()) {
        bFoundSomeInstructions = true;
        if (proto.GetPeriod() == "am") {
            ParsedDatetime_.Period = TParseDateTime::PeriodAM;
        } else if (proto.GetPeriod() == "pm") {
            ParsedDatetime_.Period = TParseDateTime::PeriodPM;
        } else if (proto.GetPeriod() != "") {
            return false;
        }
    }
    return bFoundSomeInstructions;
}

//
// Internal function
// Parse data from protobuf object
//
bool TSysDatetimeParser::ParseInternal(const TSysDateValue& proto) {
    bool bFoundSomeInstructions = false;

    if (proto.HasYears()) {
        bFoundSomeInstructions = true;
        if (proto.HasYearsRelative() && proto.GetYearsRelative()) {
            ParsedDatetime_.Year.SetRelative(proto.GetYears());
        } else {
            ParsedDatetime_.Year.SetAbsolute(proto.GetYears());
        }
    }
    if (proto.HasMonths()) {
        bFoundSomeInstructions = true;
        if (proto.HasMonthsRelative() && proto.GetMonthsRelative()) {
            ParsedDatetime_.Month.SetRelative(proto.GetMonths());
        } else {
            ParsedDatetime_.Month.SetAbsolute(proto.GetMonths());
        }
    }
    if (proto.HasDays()) {
        bFoundSomeInstructions = true;
        if (proto.HasDaysRelative() && proto.GetDaysRelative()) {
            ParsedDatetime_.Day.SetRelative(proto.GetDays());
        } else {
            ParsedDatetime_.Day.SetAbsolute(proto.GetDays());
        }
    }
    if (proto.HasDayOfWeek()) {
        bFoundSomeInstructions = true;
        ParsedDatetime_.DayOfWeek.SetAbsolute(proto.GetDayOfWeek());
    }
    if (proto.HasWeeksCount()) {
        bFoundSomeInstructions = true;
        if (proto.HasWeeksRelative() && proto.GetWeeksRelative()) {
            ParsedDatetime_.WeeksCount.SetRelative(proto.GetWeeksCount());
        } else {
            ParsedDatetime_.WeeksCount.SetAbsolute(proto.GetWeeksCount());
        }
    }
    return bFoundSomeInstructions;
}

} // namespace NAlice
