#include "types.h"

#include "visitors.h"

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <util/string/ascii.h>
#include <util/string/join.h>

// For details, see: https://tools.ietf.org/html/rfc5545

namespace NCalendarParser {

namespace {

constexpr TStringBuf COLON = ":";
constexpr TStringBuf COMMA = ",";
constexpr TStringBuf DQUOTE = "\"";
constexpr TStringBuf EQUALS = "=";
constexpr TStringBuf SEMICOLON = ";";

struct WeekdayAndName {
    WeekdayAndName(NDatetime::TWeekday weekday, TStringBuf name)
        : Weekday(weekday)
        , Name(name) {
    }

    NDatetime::TWeekday Weekday;
    TStringBuf Name;
};

const WeekdayAndName WEEKDAYS[] = {{NDatetime::TWeekday::monday, "MO"},    {NDatetime::TWeekday::tuesday, "TU"},
                                   {NDatetime::TWeekday::wednesday, "WE"}, {NDatetime::TWeekday::thursday, "TH"},
                                   {NDatetime::TWeekday::friday, "FR"},    {NDatetime::TWeekday::saturday, "SA"},
                                   {NDatetime::TWeekday::sunday, "SU"}};

bool IsControlChar(unsigned char c) {
    return c <= 0x08 || (c >= 0x0A && c <= 0x1F) || c == 0x7F;
}

bool IsSafeChar(unsigned char c) {
    if (IsControlChar(c))
        return false;
    if (c == '"' || c == ';' || c == ':' || c == ',')
        return false;
    return true;
}

bool IsQSafeChar(unsigned char c) {
    if (IsControlChar(c))
        return false;
    if (c == '"')
        return false;
    return true;
}

template <typename TFn>
void EatWhile(TStringBuf& line, TStringBuf& prefix, TFn&& fn) {
    size_t offset = 0;
    while (offset < line.length() && fn(line[offset]))
        ++offset;

    prefix = line.SubStr(0, offset);
    line.Skip(offset);
}

bool EatName(TStringBuf& line, TStringBuf& name) {
    EatWhile(line, name, [](unsigned char c) { return IsAsciiAlnum(c) || c == '-'; });
    return !name.empty();
}

bool EatParamValue(TStringBuf& line, TStringBuf& value) {
    if (line.SkipPrefix(DQUOTE)) {
        EatWhile(line, value, IsQSafeChar);
        return line.SkipPrefix(DQUOTE);
    }
    EatWhile(line, value, IsSafeChar);
    return true;
}

TStringBuf ToRFC5545(NDatetime::TWeekday weekday) {
    for (const auto& wn : WEEKDAYS) {
        if (wn.Weekday == weekday)
            return wn.Name;
    }

    Y_ASSERT(false);
    return "Unknown";
}

TMaybe<NDatetime::TWeekday> FromRFC5545(TStringBuf name) {
    for (const auto& wn : WEEKDAYS) {
        if (wn.Name == name)
            return wn.Weekday;
    }

    return Nothing();
}

void SerializeParam(TStringBuilder& builder, const TParam& param) {
    Y_ASSERT(!param.Values.empty());
    builder << param.Name << EQUALS;

    bool first = true;
    for (const auto& value : param.Values) {
        if (!first)
            builder << COMMA;
        first = false;

        if (std::all_of(value.begin(), value.end(), IsSafeChar))
            builder << value;
        else
            builder << DQUOTE << value << DQUOTE;
    }
}

TString SerializeParams(const TVector<TParam>& params) {
    TStringBuilder builder;

    bool first = true;
    for (const auto& param : params) {
        if (!first)
            builder << SEMICOLON;
        first = false;

        SerializeParam(builder, param);
    }

    return builder;
}

template <typename TComponent>
struct TComponentTraits;

template <>
struct TComponentTraits<TAlarm> {
    static const TStringBuf NAME;
};

// static
const TStringBuf TComponentTraits<TAlarm>::NAME = TStringBuf("VALARM");

template <>
struct TComponentTraits<TEvent> {
    static const TStringBuf NAME;
};

// static
const TStringBuf TComponentTraits<TEvent>::NAME = TStringBuf("VEVENT");

template <>
struct TComponentTraits<TToDo> {
    static const TStringBuf NAME;
};

// static
const TStringBuf TComponentTraits<TToDo>::NAME = TStringBuf("VTODO");

template <>
struct TComponentTraits<TCalendar> {
    static const TStringBuf NAME;
};

// static
const TStringBuf TComponentTraits<TCalendar>::NAME = TStringBuf("VCALENDAR");

struct TCalendarVisitor {
    static constexpr TStringBuf EOL = "\r\n";

    explicit TCalendarVisitor(TStringBuilder& builder)
        : Builder(builder) {
    }

    template <typename TValue>
    void operator()(const TVector<TValue>& values, TStringBuf /* name */) {
        for (const auto& value : values)
            (*this)(value, TStringBuf("value"));
    }

    template <typename TValue>
    void operator()(const TMaybe<TValue>& value, TStringBuf name) {
        if (value)
            (*this)(*value, name);
    }

    void operator()(const TWeeklyRecur& recur, TStringBuf name) {
        TVector<TStringBuf> days;
        for (const auto& day : recur.Weekdays)
            days.emplace_back(ToRFC5545(day));

        TString until;

        TVector<TParam> params;
        params.emplace_back("FREQ", TVector<TStringBuf>{{"WEEKLY"}});

        if (recur.Until) {
            until = TISO8601SerDes::Ser(*recur.Until);
            params.emplace_back("UNTIL", TVector<TStringBuf>{{until}});
        }

        params.emplace_back("BYDAY", days);

        (*this)(TContentLine("RRULE", SerializeParams(params)), name);
    }

    void operator()(const TStart& start, TStringBuf name) {
        VisitTime(start, "DTSTART" /* tag */, name);
    }

    void operator()(const TEnd& end, TStringBuf name) {
        VisitTime(end, "DTEND" /* tag */, name);
    }

    void operator()(const TContentLine& cl, TStringBuf /* name */) {
        Builder << cl.Name;
        if (!cl.Params.empty())
            Builder << SEMICOLON << SerializeParams(cl.Params);
        Builder << COLON << cl.Value << EOL;
    }

    template <typename TComponent>
    void operator()(const TComponent& component, TStringBuf /* name */) {
        Builder << "BEGIN:" << TComponentTraits<TComponent>::NAME << EOL;
        component.Visit(*this);
        Builder << "END:" << TComponentTraits<TComponent>::NAME << EOL;
    }

    template <typename TTime>
    void VisitTime(const TTime& time, TStringBuf tag, TStringBuf name) {
        TString when;
        TString tz;
        TVector<TParam> params;

        if (!time.TimeZone) {
            when = TISO8601SerDes::Ser(time.Time);
        } else {
            const auto& timeZone = *time.TimeZone;
            if (timeZone == NDatetime::GetUtcTimeZone()) {
                when = TISO8601SerDes::Ser(time.Time, timeZone);
            } else {
                when = TISO8601SerDes::Ser(time.Time);
                tz = timeZone.name();
                params.emplace_back("TZID", TVector<TStringBuf>{{tz}});
            }
        }

        (*this)(TContentLine(tag, when, params), name);
    }

    TStringBuilder& Builder;
};

// static
constexpr TStringBuf TCalendarVisitor::EOL;

template <typename TValue, typename TBuilder>
void ParseSingletonParam(const TParam& param, TMaybe<TValue>& value, TBuilder&& builder) {
    if (value)
        ythrow TTypesException() << param.Name << " param is specified more than once";
    if (param.Values.size() != 1)
        ythrow TTypesException() << param.Name << " param is should have exactly single value";

    value = builder(param.Values[0]);
}

template <typename TTime>
TTime TimeFromContent(TStringBuf value, const TVector<TParam>& params) {
    enum class EType { Date, DateTime };

    TMaybe<NDatetime::TTimeZone> timeZone;
    TMaybe<EType> type;

    for (const auto& param : params) {
        if (param.Name == "TZID") {
            ParseSingletonParam(param, timeZone, [](TStringBuf value) { return NDatetime::GetTimeZone(value); });
        } else if (param.Name == "VALUE") {
            ParseSingletonParam(param, type, [](TStringBuf value) {
                if (value == "DATE")
                    return EType::Date;
                if (value == "DATE-TIME")
                    return EType::DateTime;
                ythrow TTypesException() << "Invalid VALUE value";
            });
        }
    }

    if (!type)
        type = EType::DateTime;

    TStringBuf stream = value;
    Y_ASSERT(type);

    switch (*type) {
        case EType::Date: {
            const NDatetime::TCivilSecond time = TISO8601SerDes::DesDate(stream);
            if (!stream.empty())
                ythrow TTypesException() << "Garbage after the end of date";
            return TTime{time, timeZone};
        }
        case EType::DateTime: {
            const auto result = TISO8601SerDes::DesDateTime(stream);
            if (!stream.empty())
                ythrow TTypesException() << "Garbage after the end of date-time";

            switch (result.TimeZone) {
                case TISO8601SerDes::ETimeZone::UTC: {
                    if (!timeZone) {
                        timeZone = NDatetime::GetUtcTimeZone();
                    } else if (*timeZone != NDatetime::GetUtcTimeZone()) {
                        ythrow TTypesException() << "Timezones mismatch, in params specified " << timeZone->name()
                                                 << " but the time is in UTC";
                    }
                    Y_ASSERT(timeZone && *timeZone == NDatetime::GetUtcTimeZone());
                    break;
                }
                case TISO8601SerDes::ETimeZone::Unspecified: {
                    break;
                }
            }
            return TTime{result.Time, timeZone};
        }
    }
}
} // namespace

// TParam ----------------------------------------------------------------------
// static
TParam TParam::Parse(TStringBuf& line) {
    TStringBuf name;
    TVector<TStringBuf> values;

    if (!EatName(line, name))
        ythrow TTypesException() << "Empty param name";

    if (!line.SkipPrefix(EQUALS))
        ythrow TTypesException() << "No delimiter between param name \"" << name << "\" and value";

    while (true) {
        TStringBuf value;
        if (!EatParamValue(line, value))
            ythrow TTypesException() << "For param \"" << name << "\" can't parse value";
        values.push_back(value);
        if (!line.SkipPrefix(COMMA))
            break;
    }

    return TParam(name, std::move(values));
}

// TContentLine ----------------------------------------------------------------
// static
TContentLine TContentLine::Parse(TStringBuf line) {
    TStringBuf name;
    TVector<TParam> params;

    if (!EatName(line, name))
        ythrow TTypesException() << "Empty content line name";

    while (line.SkipPrefix(SEMICOLON))
        params.push_back(TParam::Parse(line));

    if (!line.SkipPrefix(COLON))
        ythrow TTypesException() << "No colon between content line name and value";

    return TContentLine(name, line, std::move(params));
}

// TStart ----------------------------------------------------------------------
// static
TStart TStart::FromContent(TStringBuf value, const TVector<TParam>& params) {
    return TimeFromContent<TStart>(value, params);
}

// TEnd ------------------------------------------------------------------------
TEnd TEnd::FromContent(TStringBuf value, const TVector<TParam>& params) {
    return TimeFromContent<TEnd>(value, params);
}

// TWeeklyRecur ----------------------------------------------------------------
// static
TWeeklyRecur TWeeklyRecur::Parse(TStringBuf line) {
    TWeeklyRecur recur;

    bool first = true;
    while (!line.empty()) {
        if (!first)
            line.SkipPrefix(SEMICOLON);
        first = false;

        TParam param = TParam::Parse(line);

        if (param.Name == "FREQ") {
            if (param.Values.size() != 1 || param.Values[0] != "WEEKLY")
                ythrow TTypesException() << "Invalid FREQ: " << JoinSeq(", ", param.Values);
        } else if (param.Name == "UNTIL") {
            if (param.Values.size() != 1)
                ythrow TTypesException() << "Invalid UNTIL: " << JoinSeq(", ", param.Values);
            TISO8601SerDes::TDateTime until;
            try {
                until = TISO8601SerDes::DesDateTime(param.Values[0]);
            } catch (const TISO8601SerDes::TException& e) {
                ythrow TTypesException() << "Can't parse until: " << e.what();
            }
            if (until.TimeZone != TISO8601SerDes::ETimeZone::UTC)
                ythrow TTypesException() << "UNTIL must be specified in UTC format";
            recur.Until = NDatetime::Convert(until.Time, NDatetime::GetUtcTimeZone());
        } else if (param.Name == "BYDAY") {
            auto& weekdays = recur.Weekdays;
            for (const auto& name : param.Values) {
                const TMaybe<NDatetime::TWeekday> day = FromRFC5545(name);
                if (!day)
                    ythrow TTypesException() << "Invalid weekday: " << name;
                weekdays.push_back(*day);
            }
        } else {
            ythrow TTypesException() << "Invalid recur param: " << param.Name;
        }
    }

    return recur;
}

// TCalendar -------------------------------------------------------------------
// static
TCalendar TCalendar::MakeDefault() {
    TCalendar calendar;
    auto& body = calendar.Body;
    body.emplace_back("VERSION", "2.0");
    body.emplace_back("PRODID", "-//Yandex LTD//NONSGML Quasar//EN");
    return calendar;
}

void TCalendar::Serialize(TStringBuilder& builder) const {
    TCalendarVisitor visitor(builder);
    visitor(*this, "this");
}

TString TCalendar::Serialize() const {
    TStringBuilder builder;
    Serialize(builder);
    return builder;
}

} // namespace NCalendarParser

namespace {

template <typename TTime>
void OutputTime(IOutputStream& o, const TTime& time, TStringBuf name) {
    o << name << " [";
    o << time.Time;
    if (time.TimeZone)
        o << ", " << time.TimeZone->name();
    o << "]";
}

} // namespace

template <>
void Out<NCalendarParser::TStart>(IOutputStream& o, const NCalendarParser::TStart& start) {
    OutputTime(o, start, "TStart");
}

template <>
void Out<NCalendarParser::TEnd>(IOutputStream& o, const NCalendarParser::TEnd& end) {
    OutputTime(o, end, "TEnd");
}
