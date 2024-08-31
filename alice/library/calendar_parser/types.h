#pragma once

#include <library/cpp/timezone_conversion/civil.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/string/builder.h>

#include <utility>

namespace NCalendarParser {

struct TTypesException : public yexception {};

struct TParam {
    template <typename TValues>
    TParam(TStringBuf name, TValues&& values)
        : Name(name)
        , Values(std::forward<TValues>(values)) {
    }

    // Parses single param from a line. Throws TTypesException in case
    // of failure, line will be left in a valid, but unspecified
    // state. In case of success, line will point to the rest of the
    // string.
    static TParam Parse(TStringBuf& line);

    bool operator==(const TParam& rhs) const {
        return Name == rhs.Name && Values == rhs.Values;
    }

    template <typename TVisitor>
    void Visit(TVisitor& visitor) const {
        visitor(Name, "name");
        visitor(Values, "values");
    }

    TStringBuf Name;
    TVector<TStringBuf> Values;
};

struct TContentLine {
    TContentLine() = default;

    TContentLine(TStringBuf name, TStringBuf value)
        : Name(name)
        , Value(value) {
    }

    template <typename TParams>
    TContentLine(TStringBuf name, TStringBuf value, TParams&& params)
        : Name(name)
        , Value(value)
        , Params(std::forward<TParams>(params)) {
    }

    // Parses content line from a line. Throws TTypesException in case
    // of failure.
    static TContentLine Parse(TStringBuf line);

    bool operator==(const TContentLine& rhs) const {
        return Name == rhs.Name && Value == rhs.Value && Params == rhs.Params;
    }

    template <typename TVisitor>
    void Visit(TVisitor& visitor) const {
        visitor(Name, "name");
        visitor(Value, "value");
        visitor(Params, "params");
    }

    TStringBuf Name;
    TStringBuf Value;
    TVector<TParam> Params;
};

struct TWeeklyRecur {
    // Parses content line from a line. Throws TTypesException in case
    // of failure.
    static TWeeklyRecur Parse(TStringBuf line);

    template <typename TVisitor>
    void Visit(TVisitor& visitor) const {
        visitor(Weekdays, "weekdays");
        visitor(Until, "until");
    }

    TVector<NDatetime::TWeekday> Weekdays;
    TMaybe<TInstant> Until;
};

struct TStart {
    explicit TStart(const NDatetime::TCivilSecond& time)
        : Time(time) {
    }

    TStart(const NDatetime::TCivilSecond& time, const NDatetime::TTimeZone& timeZone)
        : Time(time)
        , TimeZone(timeZone) {
    }

    TStart(const NDatetime::TCivilSecond& time, const TMaybe<NDatetime::TTimeZone>& timeZone)
        : Time(time)
        , TimeZone(timeZone) {
    }

    // Throws yexception in case of failure.
    static TStart FromContent(TStringBuf value, const TVector<TParam>& params);

    template <typename TVisitor>
    void Visit(TVisitor& visitor) const {
        visitor(Time, "time");
        visitor(TimeZone, "timeZone");
    }

    bool operator==(const TStart& rhs) const {
        return Time == rhs.Time && TimeZone == rhs.TimeZone;
    }

    NDatetime::TCivilSecond Time;
    TMaybe<NDatetime::TTimeZone> TimeZone;
};

struct TEnd {
    explicit TEnd(const NDatetime::TCivilSecond& time)
        : Time(time) {
    }

    TEnd(const NDatetime::TCivilSecond& time, const NDatetime::TTimeZone& timeZone)
        : Time(time)
        , TimeZone(timeZone) {
    }

    TEnd(const NDatetime::TCivilSecond& time, const TMaybe<NDatetime::TTimeZone>& timeZone)
        : Time(time)
        , TimeZone(timeZone) {
    }

    // Throws yexception in case of failure.
    static TEnd FromContent(TStringBuf value, const TVector<TParam>& params);

    template <typename TVisitor>
    void Visit(TVisitor& visitor) const {
        visitor(Time, "time");
        visitor(TimeZone, "timeZone");
    }

    bool operator==(const TEnd& rhs) const {
        return Time == rhs.Time && TimeZone == rhs.TimeZone;
    }

    NDatetime::TCivilSecond Time;
    TMaybe<NDatetime::TTimeZone> TimeZone;
};

struct TAlarm {
    template <typename TVisitor>
    void Visit(TVisitor& visitor) const {
        visitor(Body, "body");
    }

    TVector<TContentLine> Body;
};

struct TEvent {
    template <typename TVisitor>
    void Visit(TVisitor& visitor) const {
        visitor(Start, "start");
        visitor(End, "end");
        visitor(Recur, "recur");

        visitor(Body, "body");

        visitor(Alarms, "alarms");
    }

    TMaybe<TStart> Start;
    TMaybe<TEnd> End;
    TMaybe<TWeeklyRecur> Recur;

    TVector<TContentLine> Body;
    TVector<TAlarm> Alarms;
};

struct TToDo {
    template <typename TVisitor>
    void Visit(TVisitor& visitor) const {
        visitor(Start, "start");
        visitor(End, "end");
        visitor(Recur, "recur");

        visitor(Body, "body");
        visitor(Alarms, "alarms");
    }

    TMaybe<TStart> Start;
    TMaybe<TEnd> End;
    TMaybe<TWeeklyRecur> Recur;

    TVector<TContentLine> Body;
    TVector<TAlarm> Alarms;
};

struct TCalendar {
    static TCalendar MakeDefault();

    void Swap(TCalendar& rhs) {
        Body.swap(rhs.Body);
        Events.swap(rhs.Events);
        ToDos.swap(rhs.ToDos);
    }

    template <typename TVisitor>
    void Visit(TVisitor& visitor) const {
        visitor(Body, "body");
        visitor(Events, "events");
        visitor(ToDos, "todos");
    }

    void Serialize(TStringBuilder& builder) const;
    TString Serialize() const;

    TVector<TContentLine> Body;
    TVector<TEvent> Events;
    TVector<TToDo> ToDos;
};

} // namespace NCalendarParser
