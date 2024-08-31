#pragma once

#include "reader.h"

#include <library/cpp/scheme/scheme.h>

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/input.h>
#include <util/system/types.h>

namespace NCalendarParser {

struct TAlarm;
struct TCalendar;
struct TContentLine;

class TParser {
public:
    enum class EComponent {
        Calendar /* "VCALENDAR" */,
        Event /* "VEVENT" */,
        ToDo /* "VTODO" */,
        Journal /* "VJOURNAL" */,
        FreeBusy /* "VFREEBUSY" */,
        Alarm /* "VALARM" */,
        TimeZone /* "VTIMEZONE" */,

        // Fake component, should be last, as it's also used to count
        // number of components.  The name is empty because it don't
        // conflict with valid content line values.
        Root /* "" */
    };

    static_assert(static_cast<int>(EComponent::Root) < 64, "Too many components, revisit masks logic!");

    struct TException : public yexception {};

    // Parses whole iCalendar description from the input to the calendar.
    // In case of errors, throws TException and value is not modified.
    // Otherwise, calendar will contain parsed iCalendar description.
    //
    // NOTE: TCalendar and it's siblings are quite lightweight, as
    // they contain TStringBufs, but ensure that lines are alive
    // during usage of calendar.
    void Parse(IInputStream& input, TCalendar& calendar, TVector<TString>& lines);
    void Parse(TStringBuf data, TCalendar& calendar, TVector<TString>& lines);

private:
    bool NextLine(TContentLine& cl);

    bool EatBegin(const TContentLine& cl, EComponent current, EComponent& child);
    bool EatEnd(const TContentLine& cl, EComponent current);

    void ParseCalendar(TCalendar& calendar);

    template <typename TComponent>
    void ParseNodeComponent(EComponent current, TComponent& component);

    template <typename TComponent>
    void ParseLeafComponent(EComponent current, TComponent& component);

    EComponent GetComponent(TStringBuf name) const;
    static bool CheckParentChild(EComponent parent, EComponent child);

    void Panic(TStringBuf message) const;

    THolder<TReader> Reader;
    i64 LineNumber = -1;
    TVector<TString> Lines;
};

} // namespace NCalendarParser
