#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {

    // week days
    constexpr TStringBuf EXCEPT_WEEK_DAYS("except_week_days");
    constexpr TStringBuf WEEK_DAYS_START("week_days_start");
    constexpr TStringBuf WEEK_DAYS_END("week_days_end");
    constexpr TStringBuf WEEK_DAYS("week_days");
    constexpr TStringBuf WEEK_DAY("week_day");
    constexpr TStringBuf WEEK_DAYS_RESULT("weekdays");
    // time
    constexpr TStringBuf HOURS("hours");
    constexpr TStringBuf HOURS_RELATIVE("hours_relative");
    constexpr TStringBuf MINUTES("minutes");
    constexpr TStringBuf MINUTES_RELATIVE("minutes_relative");
    constexpr TStringBuf SECONDS("seconds");
    constexpr TStringBuf SECONDS_RELATIVE("seconds_relative");
    constexpr TStringBuf DAY_PART("day_part");
    constexpr TStringBuf PERIOD("period");
    // date
    constexpr TStringBuf DAYS("days");
    constexpr TStringBuf DAYS_RELATIVE("days_relative");
    constexpr TStringBuf MONTHS("months");
    constexpr TStringBuf MONTHS_RELATIVE("months_relative");
    constexpr TStringBuf YEARS("years");
    constexpr TStringBuf YEARS_RELATIVE("years_relative");
    constexpr TStringBuf WEEKS("weeks");
    constexpr TStringBuf WEEKS_RELATIVE("weeks_relative");
    constexpr TStringBuf WEEKEND{"weekend"};
    // number
    constexpr TStringBuf NUMBER("number");
    // float
    constexpr TStringBuf FLOAT_VALUE("float_value");
    constexpr TStringBuf MULTIPLIER("multiplier");
    constexpr TStringBuf NUMBER_DIVISOR("number_divisor");
    constexpr TStringBuf POWER_DIVISOR("power_divisor");
    // datetime
    constexpr TStringBuf DATE("date");
    constexpr TStringBuf TIME("time");
    // datetime_range
    constexpr TStringBuf DATETIME_RANGE_START("start");
    constexpr TStringBuf DATETIME_RANGE_END("end");
    // general
    constexpr TStringBuf IS_RELATIVE("is_relative");
    constexpr TStringBuf REPEAT("repeat");
    constexpr TStringBuf BEGIN("span_begin");
    constexpr TStringBuf END("span_end");
    constexpr TStringBuf TEXT("text");
    constexpr TStringBuf CONTENT("content");
    // keys to filter entities and decide what type of objects to define
    const TVector<TStringBuf> WEEK_DAYS_LIST({WEEK_DAYS, WEEK_DAYS_START, WEEK_DAYS_END, EXCEPT_WEEK_DAYS});
    const TVector<TStringBuf> DATE_LIST({DAYS, MONTHS, YEARS, WEEKS, WEEK_DAY});
    const TVector<TStringBuf> TIME_LIST({HOURS, MINUTES, SECONDS});
    const TVector<TStringBuf> NUMBER_LIST({NUMBER});
    const TVector<TStringBuf> FLOAT_LIST({NUMBER, NUMBER_DIVISOR, POWER_DIVISOR});
    const TVector<TStringBuf> DATETIME_LIST({DATE, TIME});
    const TVector<TStringBuf> DATETIME_RANGE_LIST({DATETIME_RANGE_START, DATETIME_RANGE_END});
} // namespace NAlice
