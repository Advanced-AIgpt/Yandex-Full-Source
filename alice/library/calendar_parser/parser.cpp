#include "parser.h"

#include "types.h"

#include <util/stream/mem.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/system/yassert.h>

namespace NCalendarParser {

namespace {
constexpr TStringBuf BEGIN = "BEGIN";
constexpr TStringBuf END = "END";

constexpr ui64 ToMask(TParser::EComponent component) {
    return static_cast<ui64>(1) << static_cast<int>(component);
}

template <typename... TComponents>
constexpr ui64 ToMask(TParser::EComponent component, TComponents... components) {
    return (static_cast<ui64>(1) << static_cast<int>(component)) | ToMask(components...);
}

constexpr ui64 CALENDAR_CHILDREN =
    ToMask(TParser::EComponent::Event, TParser::EComponent::ToDo, TParser::EComponent::Journal,
           TParser::EComponent::FreeBusy, TParser::EComponent::TimeZone);

// Dummy structure, used to parse leaf components, such as VTIMEZONE.
// We don't need them (actually, only VEVENT and VTODO are in use),
// therefore they're not exposed.
struct TLeafComponent {
    TVector<TContentLine> Body;
};
} // namespace

void TParser::Parse(IInputStream& input, TCalendar& calendar, TVector<TString>& lines) {
    const EComponent current = EComponent::Root;

    Reader = MakeHolder<TReader>(input);
    LineNumber = -1;
    Lines.clear();

    try {
        TContentLine cl;
        EComponent child;
        if (!NextLine(cl))
            Panic("Calendar description is empty");
        if (!EatBegin(cl, current, child))
            Panic("First line must be BEGIN:VCALENDAR");

        TCalendar c;
        ParseCalendar(c);
        calendar.Swap(c);
        lines.swap(Lines);
    } catch (const yexception& e) {
        Panic(e.what());
    }
}

void TParser::Parse(TStringBuf data, TCalendar& calendar, TVector<TString>& lines) {
    TMemoryInput input(data);
    return Parse(input, calendar, lines);
}

bool TParser::NextLine(TContentLine& cl) {
    Y_ASSERT(Reader);

    Lines.emplace_back();
    const bool success = Reader->NextLine(Lines.back());
    if (success) {
        ++LineNumber;
        cl = TContentLine::Parse(Lines.back());
    }
    return success;
}

bool TParser::EatBegin(const TContentLine& cl, EComponent current, EComponent& child) {
    if (cl.Name != BEGIN)
        return false;

    child = GetComponent(cl.Value);
    if (!CheckParentChild(current, child))
        Panic(TStringBuilder() << "Component " << child << " can't be nested in " << current);

    if (!cl.Params.empty())
        Panic(TStringBuilder() << "Non-empty param list for BEGIN: " << cl.Value);

    return true;
}

bool TParser::EatEnd(const TContentLine& cl, EComponent current) {
    if (cl.Name != END)
        return false;

    const EComponent component = GetComponent(cl.Value);
    if (component != current)
        Panic(TStringBuilder() << "Mismatched END, expected: " << current << ", actual: " << component);

    return true;
}

void TParser::ParseCalendar(TCalendar& calendar) {
    const EComponent current = EComponent::Calendar;

    TContentLine cl;
    EComponent child;
    while (NextLine(cl)) {
        if (EatBegin(cl, current, child)) {
            switch (child) {
                case EComponent::Event:
                    calendar.Events.emplace_back();
                    ParseNodeComponent(child, calendar.Events.back());
                    break;
                case EComponent::ToDo:
                    calendar.ToDos.emplace_back();
                    ParseNodeComponent(child, calendar.ToDos.back());
                    break;
                default:
                    TLeafComponent component;
                    ParseLeafComponent(child, component);
                    break;
            }
        } else if (EatEnd(cl, current)) {
            break;
        } else {
            calendar.Body.push_back(std::move(cl));
        }
    }
}

template <typename TComponent>
void TParser::ParseNodeComponent(EComponent current, TComponent& component) {
    TContentLine cl;
    EComponent child;

    while (NextLine(cl)) {
        if (EatBegin(cl, current, child)) {
            Y_ASSERT(child == EComponent::Alarm);
            component.Alarms.emplace_back();
            ParseLeafComponent(child, component.Alarms.back());
        } else if (EatEnd(cl, current)) {
            break;
        } else {
            if (cl.Name == "DTSTART")
                component.Start = TStart::FromContent(cl.Value, cl.Params);
            else if (cl.Name == "DTEND")
                component.End = TEnd::FromContent(cl.Value, cl.Params);
            else if (cl.Name == "RRULE")
                component.Recur = TWeeklyRecur::Parse(cl.Value);
            else
                component.Body.push_back(std::move(cl));
        }
    }
}

template <typename TComponent>
void TParser::ParseLeafComponent(EComponent current, TComponent& component) {
    TContentLine cl;
    EComponent child;

    while (NextLine(cl)) {
        if (EatBegin(cl, current, child))
            Y_ASSERT(false);
        else if (EatEnd(cl, current))
            break;
        else
            component.Body.push_back(std::move(cl));
    }
}

TParser::EComponent TParser::GetComponent(TStringBuf name) const {
    EComponent component;
    if (!TryFromString<EComponent>(name, component))
        Panic(TStringBuilder() << "Unknown component: " << name);
    return component;
}

// static
bool TParser::CheckParentChild(EComponent parent, EComponent child) {
    switch (parent) {
        case EComponent::Root:
            return child == EComponent::Calendar;
        case EComponent::Calendar:
            return (ToMask(child) & CALENDAR_CHILDREN) != 0;
        case EComponent::Event:
            [[fallthrough]];
        case EComponent::ToDo:
            return child == EComponent::Alarm;
        case EComponent::Journal:
            [[fallthrough]];
        case EComponent::FreeBusy:
            [[fallthrough]];
        case EComponent::Alarm:
            [[fallthrough]];
        case EComponent::TimeZone:
            return false;
    }
}

void TParser::Panic(TStringBuf message) const {
    ythrow TException() << "Line " << LineNumber << ": " << message;
}

} // namespace NCalendarParser

template <>
void Out<NCalendarParser::TParser::TException>(IOutputStream& out,
                                               const NCalendarParser::TParser::TException& e) {
    out << static_cast<const yexception&>(e);
}
