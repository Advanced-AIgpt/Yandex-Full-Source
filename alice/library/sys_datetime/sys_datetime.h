#pragma once

#include <alice/protos/data/entities/datetime.pb.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/timezone_conversion/civil.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/maybe.h>

namespace NAlice {

///
/// TSysDatetimeParser - class to parse following NLU objects:
///    sys.date
///    sys.datetime
///    sys.time
///
class TSysDatetimeParser {
public:
    //
    // Internal structure to keep parsed data
    //
    struct TParseDateTime {
        //
        // Int value wrapper with 'Not Set', 'Absolute', 'Relative' values
        //
        class TValue {
        public:
            enum EValueInfo {
                ValueInfoNotFilled,
                ValueInfoAbsolute,
                ValueInfoRelative
            };

            inline EValueInfo GetValueInfo() const {
                return Info_;
            }
            inline bool IsFilled() const {
                return GetValueInfo() != ValueInfoNotFilled;
            }
            inline bool IsNotFilled() const {
                return GetValueInfo() == ValueInfoNotFilled;
            }
            inline bool IsAbsolute() const {
                return GetValueInfo() == ValueInfoAbsolute;
            }
            inline bool IsRelative() const {
                return GetValueInfo() == ValueInfoRelative;
            }
            inline int Get() const {
                Y_ENSURE(Info_ != ValueInfoNotFilled);
                return Value_;
            }
            inline void SetAbsolute(int value) {
                Value_ = value;
                Info_ = ValueInfoAbsolute;
            }
            inline void SetRelative(int value) {
                Value_ = value;
                Info_ = ValueInfoRelative;
            }
            inline void Erase() {
                Info_ = ValueInfoNotFilled;
            }
            bool operator == (const TValue& rhs) const {
                return Value_ == rhs.Value_ && Info_ == rhs.Info_;
            }

        private:
            int Value_ = 0;
            EValueInfo Info_ = ValueInfoNotFilled;
        };

        enum EPeriod {
            PeriodNone,
            PeriodAM,
            PeriodPM
        };

        TValue Year;      // usually 2021, etc
        TValue Month;     // 1...12
        TValue Day;       // 1...31
        TValue Hour;      // 0...23 (or 0...11 if Period = PeriodAM/PM)
        TValue Minutes;   // 0...59
        TValue Seconds;   // 0...59
        TValue WeeksCount;// only relative
        TValue DayOfWeek; // 0...6 from Sunday, only absolute
        EPeriod Period = PeriodNone;

        // Internal field accessors
        TParseDateTime::TValue& At(TParseDateTime::TValue TParseDateTime::*Var) {
            return this->*Var;
        }
        TParseDateTime::TValue At(TParseDateTime::TValue TParseDateTime::*const Var) const {
            return (this->*Var);
        }
        bool Merge(const TParseDateTime& rhs);

        bool operator == (const TParseDateTime& rhs) const {
            return Year == rhs.Year && Month == rhs.Month && Day == rhs.Day &&
                   Hour == rhs.Hour && Minutes == rhs.Minutes && Seconds == rhs.Seconds &&
                   WeeksCount == rhs.WeeksCount && DayOfWeek == rhs.DayOfWeek && 
                   Period == rhs.Period;
        }
    };

    enum class EDatetimeParser {
        Unknown,        // This slot doesn't contain any valid date or time keys
        Fixed,          // This slot contains date and/or time information
        RelativeFuture, // This slot contains relative date (future)
        RelativePast,   // This slot contains relative date (past)
        RelativeMix,    // This slot contains mixed relative date
        FixedDayOfWeek, // This slot contains day of week info only
        Mix             // This slot contains mixed data (i.e. both relative and fixed)
    };

    enum class EDatetimeContent {
        Unknown,                      // Can't detect any date/time data
        DateOnly,                     // This class contains date only fields
        TimeOnly,                     // This class contains time only fields
        DateTime                      // This class contains both date and time fields
    };

    // Parse from JSON
    static TMaybe<TSysDatetimeParser> ParseJson(const NJson::TJsonValue& slotValue);
    static TMaybe<TSysDatetimeParser> Parse(const TString& slotValue);
    // Parse from PROTO
    static TMaybe<TSysDatetimeParser> Parse(const TSysDatetimeValue& proto);
    static TMaybe<TSysDatetimeParser> Parse(const TSysTimeValue& proto);
    static TMaybe<TSysDatetimeParser> Parse(const TSysDateValue& proto);
    // Special case
    static TMaybe<TSysDatetimeParser> Today();
    static TMaybe<TSysDatetimeParser> Yesterday();
    static TMaybe<TSysDatetimeParser> Tomorrow();

    // Get as Json
    NJson::TJsonValue GetAsJsonDatetime() const;
    TSysDatetimeValue GetAsProtoDatetime() const;
    // Get as a PROTO
    TSysTimeValue GetAsProtoTime() const;
    TSysDateValue GetAsProtoDate() const;

    //
    // Get preliminary parsed info
    //
    EDatetimeParser GetParseInfo() const;
    EDatetimeContent GetParseContent() const;
    inline const TParseDateTime& GetRawDatetime() const {
        return ParsedDatetime_;
    }
    inline TParseDateTime& GetRawDatetime() {
        return ParsedDatetime_;
    }

    inline bool Merge(const TSysDatetimeParser& rhs) {
        return ParsedDatetime_.Merge(rhs.ParsedDatetime_);
    }

    //
    // Get an absolute date/time from the slot
    //
    enum ETense {
        TenseDefault, // Use suggestions from the slot info (i.e. relative:-5 == past)
        TenseFuture,  // Use future tense if possible
        TensePast     // Use past tense if possible
    };
    NDatetime::TCivilSecond GetTargetDateTime(const NDatetime::TCivilSecond& dateCurrent, ETense tense) const;

    ///
    /// Set fixup value for year correction (i.e. 21 -> 2021)
    /// This function allows you to modify default range for year correction when tense is `TenseDefault`.
    /// For example, current year is 2020 and standard value is 40.
    /// It means thats years "20", "21", "60" will be converted to 2020, 2021, 2060
    /// All other values will be converted to past (i.e 0 -> 2000, 12 -> 2012, 61 -> 1961, 99 -> 1999)
    ///
    void SetDefautYearCorrection(int yearsForward) {
        DefaultYearCorrection_ = yearsForward;
    }

    ///
    /// Set 'human-hours' correction
    /// With 'human-hours' correction relative requests like 'Tomorrow' will return CURRENT date for requests made from 0:00 am till specified time
    /// Example: 'what wearther will be tomorrow' asked 24/01/2022 0:29:
    ///     will return 25/01/2022 without 'human-hours' correction
    ///     will return 24/01/2022 (actually today) with 'human-hours' correction set to 30 min or more
    ///
    void SetHumanCorrection(TDuration timeAfterMidnight) {
        HumanLagAfterMidnight_ = timeAfterMidnight;
    }

private:
    TSysDatetimeParser() {}
    bool ParseInternal(const NJson::TJsonValue& slotValue);
    bool ParseInternal(const TSysTimeValue& proto);
    bool ParseInternal(const TSysDateValue& proto);

    TParseDateTime ParsedDatetime_;
    int DefaultYearCorrection_ = 40;
    TDuration HumanLagAfterMidnight_ = TDuration::Zero();
};

} // namespace NAlice
