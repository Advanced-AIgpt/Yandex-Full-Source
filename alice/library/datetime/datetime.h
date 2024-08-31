#pragma once

#include <library/cpp/timezone_conversion/civil.h>
#include <library/cpp/timezone_conversion/convert.h>
#include <library/cpp/scheme/scheme.h>
#include <util/generic/vector.h>
#include <util/generic/maybe.h>

namespace NAlice {

class TDaysParser;

/** Class defines the specific date(time) plus day_part (evening, night, morning, day, whole_day)
 */
class TDateTime {
public:
    /** The time broken into parts such as YMDhms, tz, offset, etc...
     * Right now it is a TSimpleTM with the knowledge of TimeZone (since TSimpleTM doesn't know
     * anything about timezone, just offset, therefore any calculation can be incorrect)
     * Later there will (library/cpp/timezone_conversion) be a class which is the same intercase
     * but different implementation based on cctz.
     */
    class TSplitTime {
    public:
        enum class EField {
            F_NONE = 0,
            F_SEC,
            F_MIN,
            F_HOUR,
            F_DAY,
            F_MON,
            F_YEAR
        };


        /** Construct a proper broken down time from epoch with respect the timezone.
         * @param[in] tz is a timezone object
         * @param[in] epoch is a time in seconds from the time beginning
         */
        TSplitTime(const NDatetime::TTimeZone& tz, time_t epoch);
        /** Construct a proper broken down time from a given YMDhms with respect the timezone.
         * @param[in] tz is a timezone object
         * @param[in] year is a valid year (not a difference between year you want and 1900)
         * @param[in] month is a real month (not a month you want minus one!)
         */
        TSplitTime(const NDatetime::TTimeZone& tz, ui32 year, ui32 mon, ui32 day, ui32 h = 0, ui32 m = 0, ui32 s = 0);

        /** Add component (year, month, etc) to date/time
        * @param[in] f is a type of component (from enum)
        * @param[in] amount - how much to add
        * returns reference to this object
        */
        TSplitTime& Add(EField f, i32 amount = 1);

        /** Accessors to components */
        const NDatetime::TTimeZone& TimeZone() const {
            return TZ;
        }

        int RealYear() const {
            return Time.year();
        }

        int RealMonth() const {
            return Time.month();
        }

        int MDay() const {
            return Time.day();
        }

        int YDay() const {
            return NDatetime::GetYearDay(NDatetime::TCivilDay(Time));
        }

        int Hour() const {
            return Time.hour();
        }

        int Min() const {
            return Time.minute();
        }

        int Sec() const {
            return Time.second();
        }

        int WDay() const {
            NDatetime::TWeekday wday = NDatetime::GetWeekday(NDatetime::TCivilDay(Time));
            if (wday == NDatetime::TWeekday::sunday) {
                return 0;
            } else {
                return static_cast<int>(wday) + 1;
            }
        }

        /** returns seconds since 1970, for older dates - 0 */
        //FIXME: remove AsTimeT()!!! ASSISTANT-3016
        ui64 AsTimeT() const;

        TString ToString(const char* fmt = "%a, %d %b %Y %H:%M:%S %z") const {
            return NDatetime::Format(fmt, Time, TZ);
        }

        void SetDay(int day) {
            Time = NDatetime::TCivilSecond{
                Time.year(),
                Time.month(),
                day,
                Time.hour(),
                Time.minute(),
                Time.second()
            };
        }

    private:
        NDatetime::TCivilSecond Time;
        NDatetime::TTimeZone TZ;

    };

    enum class EDayPart : ui8 {
        Night    = 0,
        Morning  = 1,
        Day      = 2,
        Evening  = 3,
        WholeDay = 4,
        Invalid  = 5,
    };


public:
    /** Create object with particular datetime and tries to determine daypart from time
     * by using the following rule:
     * @code
     * [00:00 - 06:00) - night
     * [06:00 - 12:00) - morning
     * [12:00 - 18:00) - day
     * [18:00 - 24:00) - evening
     * @endcode
     * This constructor will never create DayPart as #EDayPart::WholeDay.
     */
    explicit TDateTime(const TSplitTime& st);

    /** Create object with particular datetime and a string representation of dayPart.
     * Also it calculates offset with userTime.
     * DayPart sets to #EDayPart::Invalid in case string contains unknown day part.
     * @see DayPartAsString()
     */
    TDateTime(const TSplitTime& st, TStringBuf dayPart);
    TDateTime(const TSplitTime& st, EDayPart dayPart);

    /** It returns offset in days this object and the given one
     * @param[in] ut is a time which substracted from this object
     * @return days (could be negative) the difference between this object and the #ut
     */
    ssize_t OffsetWidth(const TSplitTime& ut) const;

    ssize_t OffsetWidth(const TDateTime& ut) const {
        return OffsetWidth(ut.SplitTime());
    }

    /** Returns a requested daypart.
     */
    EDayPart DayPart() const {
        return CurrentDayPart;
    }

    /** Returns a string representation of day part.
     * ("night", "day", "evening", "morning", "whole_day" are valid string represetnations)
     * @see DayPart()
     */
    TStringBuf DayPartAsString() const;

    const TSplitTime& SplitTime() const {
        return ST;
    }

    static TStringBuf DayPartAsString(EDayPart dp);
    static EDayPart StringToDayPart(TStringBuf dp);
    static EDayPart TimeToDayPart(const TSplitTime& st);

    /** Returns if day part one of the follwing (night, morning, day, eventing)
     */
    bool IsDayPartExplicit() const {
        return CurrentDayPart != EDayPart::WholeDay && CurrentDayPart != EDayPart::Invalid;
    }

private:
    const EDayPart CurrentDayPart;
    const TSplitTime ST;
};

bool operator <(TDateTime::EDayPart lhs, TDateTime::EDayPart rhs);
bool operator >(TDateTime::EDayPart lhs, TDateTime::EDayPart rhs);

/** Some additional params to pass to functions which create TDateTimeList object.
 */
struct TDateTimeListCreationParams {
    /** Maximum amounts of days to create (ie weather can only have 10
     * days forecast, so we restrict to 10 days)
     */
    size_t MaxDays = 1;
    /** In case requested today and the time (only time, not date) in the past then
     * increment the day. It needs for the situations when user asks 'today midnight' and
     * it is the midnight of the next day not the past one
     * @default is false
     */
    bool LookForward = false;
};

class TDaysParser {
public:
    TDaysParser(const TDateTimeListCreationParams* params, bool isRange)
        : IsRange(isRange)
        , ExternalParams(params)
        , DefaultParams({ 1, false })
    {
    }

    TDateTime ParseOneDay(const NSc::TValue& v,
                          const TDateTime& ut,
                          const TDateTime::EDayPart* dayPartFromSlot,
                          const TDateTime::TSplitTime* leftBound = nullptr);

    TDateTime ParseOneDay(const TString& v,
                          const TDateTime& ut,
                          const TDateTime::EDayPart* dayPartFromSlot,
                          const TDateTime::TSplitTime* leftBound = nullptr);

    // requested the current time (i.e. the current weather; not today weather)
    // this is modified after each call of ParseOneDay()
    bool CurrentTimeRequest = false;
    // means that there is at least one user entered absolute value in date (not in time)
    // it is needed for calculating in ranges
    // i.e. { days: 1, months: 2, months_relative: true } since days is absolute
    // this is modified after each call of ParseOneDay()
    bool HasCustomAbs = false;

private:
    using TParams = TDateTimeListCreationParams;
    const TParams& Params() const {
        return ExternalParams ? *ExternalParams : DefaultParams;
    }

private:
    const bool IsRange;
    const TParams* ExternalParams;
    const TParams DefaultParams; // TODO make it lazy (not sure)
};

/** The class represents the parsed user date input as list of parsed days. It could be in two states:
 * IsNow() == true and there is no days in there : no days defined and user requested date as NOW
 * IsNow() == false and there are one or more days : user specified the particular day or a range
 */
class TDateTimeList {
public:
    using TDaysList = TVector<TDateTime>;
    using TConstIterator = TDaysList::const_iterator;
    using TConstReverseIterator = TDaysList::const_reverse_iterator;

public:
    /** Try to create a TDateTimeList by parsing datetime_* and daypart slots with the given epoch/timezone
     * @param[in] slot the datetime|datetime_range slot (could be null)
     * @param[in] dayPart is day part slot (could be null)
     * @param[in] epoch is seconds from epoch
     * @param[in] tzName is a timezone name
     * Will throw yexception if an error happened
     */
    template<typename TSlotType>
    static std::unique_ptr<TDateTimeList> CreateFromSlot(const TSlotType* dtSlot,
                                                         const TSlotType* dayPartSlot,
                                                         const TDateTime& userTime,
                                                         const TDateTimeListCreationParams& params) {
        if (IsSlotEmpty(dtSlot)) {
            return CreateDateTime(NSc::Null(), dayPartSlot, userTime, params);
        }

        if (TStringBuf("datetime_raw") == dtSlot->Type ||
            TStringBuf("datetime") == dtSlot->Type ||
            TStringBuf("date") == dtSlot->Type)
        {
            return CreateDateTime(dtSlot->Value, dayPartSlot, userTime, params);
        }

        if (TStringBuf("datetime_range") == dtSlot->Type ||
            TStringBuf("datetime_range_raw") == dtSlot->Type)
        {
            return CreateDateTimeRange(*dtSlot, dayPartSlot, userTime, params);
        }

        ythrow yexception() << "Unknown type of datetime slot: '" << dtSlot->Type << '\'';
    }

public:
    /** Returns if object means no days but the current time!
     * @see TotalDays()
     */
    bool IsNow() const {
        return !Days;
    }

    /** Return the amount of days it contains.
     * If it is zero, it means that the date is current.
     * @see IsNow()
     */
    size_t TotalDays() const {
        return Days.size();
    }

    TConstReverseIterator rbegin() const {
        return Days.rbegin();
    }

    TConstReverseIterator rend() const {
        return Days.rend();
    }

    TConstIterator cbegin() const {
        return Days.cbegin();
    }

    TConstIterator begin() const {
        return Days.cbegin();
    }

    TConstIterator cend() const {
        return Days.cend();
    }

    TConstIterator end() const {
        return Days.cend();
    }

    template<typename TSlotType, typename TSlotValueType>
    static std::unique_ptr<TDateTimeList> CreateDateTime(const TSlotValueType& dtSlotValue,
                                                         const TSlotType* dayPartSlot,
                                                         const TDateTime& userTime,
                                                         const TDateTimeListCreationParams& params) {
        TMaybe<TDateTime::EDayPart> dayPartFromSlot;
        if (!IsSlotEmpty(dayPartSlot)) {
            dayPartFromSlot = TDateTime::StringToDayPart(GetSlotString(dayPartSlot));
            if (*dayPartFromSlot == TDateTime::EDayPart::Invalid) {
                ythrow yexception() << "Unknown type of datetime slot: '" << *dayPartFromSlot;
            }
        }

        TDaysParser parser(&params, false);

        TDateTime adjusted = parser.ParseOneDay(
            dtSlotValue, userTime,
            dayPartFromSlot ? dayPartFromSlot.Get() : nullptr
        );

        // clear flag FOR_NOW in case there is a good dayPart slot
        if (dayPartFromSlot) {
            parser.CurrentTimeRequest = false;
        }

        return std::unique_ptr<TDateTimeList>(parser.CurrentTimeRequest ? new TDateTimeList : new TDateTimeList(adjusted));
    }

    template<typename TSlotType>
    static std::unique_ptr<TDateTimeList> CreateDateTimeRange(const TSlotType& dtSlot,
                                                              const TSlotType* dayPartSlot,
                                                              const TDateTime& userTime,
                                                              const TDateTimeListCreationParams& params) {
        if (IsSlotEmpty(&dtSlot)) {
            return std::unique_ptr<TDateTimeList>(new TDateTimeList);
        }

        TDateTime::EDayPart dp;
        if (!IsSlotEmpty(dayPartSlot)) {
            dp = TDateTime::StringToDayPart(GetSlotString(dayPartSlot));
            if (dp == TDateTime::EDayPart::Invalid) {
                ythrow yexception() << "Invalid day_part slot value: " << dp;
            }
        } else {
            dp = TDateTime::EDayPart::WholeDay;
        }

        std::unique_ptr<TDateTimeList> dtl;

        TDaysParser parser(&params, true);

        NSc::TValue slotValue;
        if constexpr (std::is_same_v<TString, decltype(dtSlot.Value)>) {
            slotValue = NSc::TValue::FromJson(dtSlot.Value);
        } else {
            slotValue = dtSlot.Value;
        }

        const NSc::TValue& startSrc = slotValue["start"];
        TDateTime::TSplitTime start = parser.ParseOneDay(startSrc, userTime, &dp).SplitTime();
        // XXX странно, но таки да, ищем ближайшие выходные от найденой даты
        // повезло если число совпало с выходным :)
        const NSc::TValue& weekend = startSrc["weekend"];
        if (!weekend.IsNull()) {
            if (start.WDay() > 0 && start.WDay() < 6) {
                start.Add(TDateTime::TSplitTime::EField::F_DAY, 6 - start.WDay());
            }
            dtl.reset(new TDateTimeList(TDateTime(start, dp)));

            // считаем что если start == weekend, то и end тоже weekend (т.е. проверять не буду)
            if (start.WDay() == 6) {
                start.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
                dtl->Days.emplace_back(start, dp);
            }
        }
        else {
            dtl.reset(new TDateTimeList(TDateTime(start, dp)));
            TDateTime::TSplitTime end = parser.ParseOneDay(slotValue["end"], userTime, &dp, &start).SplitTime();
            for (size_t i = 0; i < params.MaxDays; ++i) {
                start.Add(TDateTime::TSplitTime::EField::F_DAY, 1);

                //FIXME: remove AsTimeT()!!!
                if (start.AsTimeT() > end.AsTimeT() || (!parser.HasCustomAbs && start.AsTimeT() == end.AsTimeT())) {
                    break;
                }

                dtl->Days.push_back(TDateTime(start, dp));
            }
        }

        return dtl;
    }

private:
    TDateTimeList();
    explicit TDateTimeList(const TDateTime& dt);

    // TODO(sparkle): assume that slot->Value is only TString after BASS is dead
    template<typename TSlotType>
    static bool IsSlotEmpty(const TSlotType* slot) {
        if (!slot) {
            return true;
        }

        constexpr bool isString = std::is_same_v<TString, decltype(slot->Value)>;
        constexpr bool isJson = std::is_same_v<NSc::TValue, decltype(slot->Value)>;
        static_assert(isString || isJson);

        if constexpr (isString) {
            return slot->Value == "null";
        } else {
            return slot->Value.IsNull();
        }
    }

    template<typename TSlotType>
    static TStringBuf GetSlotString(const TSlotType* slot) {
        Y_ASSERT(slot);

        constexpr bool isString = std::is_same_v<TString, decltype(slot->Value)>;
        constexpr bool isJson = std::is_same_v<NSc::TValue, decltype(slot->Value)>;
        static_assert(isString || isJson);

        if constexpr (isString) {
            return slot->Value;
        } else {
            return slot->Value.GetString();
        }
    }

private:
    TDaysList Days;
};

}
