#include "parser.h"

#include "types.h"
#include "visitors.h"

#include <alice/library/unittest/ut_helpers.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

using namespace NCalendarParser;
using namespace NTestingHelpers;

namespace {
// These examples are taken from: https://tools.ietf.org/html/rfc5545

constexpr TStringBuf SOURCE_0 = TStringBuf(R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//hacksw/handcal//NONSGML v1.0//EN
BEGIN:VEVENT
UID:19970610T172345Z-AF23B2@example.com
DTSTAMP:19970610T172345Z
DTSTART:19970714T170000Z
DTEND:19970715T040000Z
SUMMARY:Bastille Day Party
END:VEVENT
END:VCALENDAR
)");

const NSc::TValue EXPECTED_0 = NSc::TValue::FromJson(R"(
{
    "body" : [
        {"name" : "VERSION", "value" : "2.0"},
        {"name" : "PRODID", "value" : "-//hacksw/handcal//NONSGML v1.0//EN"}
    ],
    "events" : [
        {
            "body" : [
                {"name" : "UID", "value" : "19970610T172345Z-AF23B2@example.com"},
                {"name" : "DTSTAMP", "value" : "19970610T172345Z"},
                {"name" : "SUMMARY", "value" : "Bastille Day Party"}
            ],
            "end" : {
                "time" : "1997-07-15T04:00:00",
                "timeZone": "UTC"
            },
            "start" : {
                "time" : "1997-07-14T17:00:00",
                "timeZone": "UTC"
            }
        }
    ]
}
)");

constexpr TStringBuf SOURCE_1 = TStringBuf(R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//ABC Corporation//NONSGML My Product//EN
BEGIN:VTODO
DTSTAMP:19980130T134500Z
SEQUENCE:2
UID:uid4@example.com
ORGANIZER:mailto:unclesam@example.com
ATTENDEE;PARTSTAT=ACCEPTED:mailto:jqpublic@example.com
DUE:19980415T000000
STATUS:NEEDS-ACTION
SUMMARY:Submit Income Taxes
BEGIN:VALARM
ACTION:AUDIO
TRIGGER:19980403T120000Z
ATTACH;FMTTYPE=audio/basic:http://example.com/pub/audio-
 files/ssbanner.aud
REPEAT:4
DURATION:PT1H
END:VALARM
END:VTODO
END:VCALENDAR
)");

const NSc::TValue EXPECTED_1 = NSc::TValue::FromJson(R"(
{
    "body" : [
        {"name" : "VERSION", "value" : "2.0"},
        {"name" : "PRODID", "value" : "-//ABC Corporation//NONSGML My Product//EN"}
    ],
    "todos" : [
        {
            "body" : [
                {"name" : "DTSTAMP", "value" : "19980130T134500Z"},
                {"name" : "SEQUENCE", "value" : "2"},
                {"name" : "UID", "value" : "uid4@example.com"},
                {"name" : "ORGANIZER", "value" : "mailto:unclesam@example.com"},
                {
                    "name" : "ATTENDEE",
                    "params" : [{"name" : "PARTSTAT", "values" : ["ACCEPTED"]}],
                    "value" : "mailto:jqpublic@example.com"
                },
                {"name" : "DUE", "value" : "19980415T000000"},
                {"name" : "STATUS", "value" : "NEEDS-ACTION"},
                {"name" : "SUMMARY", "value" : "Submit Income Taxes"}
            ],
            "alarms" : [
                {
                    "body" : [{"name" : "ACTION", "value" : "AUDIO"},
                              {"name" : "TRIGGER", "value" : "19980403T120000Z"},
                              {
                                  "name" : "ATTACH",
                                  "params" : [{"name" : "FMTTYPE", "values" : ["audio/basic"]}],
                                  "value" : "http://example.com/pub/audio-files/ssbanner.aud"
                              },
                              {"name" : "REPEAT", "value" : "4"},
                              {"name" : "DURATION", "value" : "PT1H"}
                    ]
                }
            ]
        }
    ]
}
)");

constexpr TStringBuf SOURCE_2 = TStringBuf(R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//RDU Software//NONSGML HandCal//EN
BEGIN:VFREEBUSY
ORGANIZER:mailto:jsmith@example.com
DTSTART:19980313T141711Z
DTEND:19980410T141711Z
FREEBUSY:19980314T233000Z/19980315T003000Z
FREEBUSY:19980316T153000Z/19980316T163000Z
FREEBUSY:19980318T030000Z/19980318T040000Z
URL:http://www.example.com/calendar/busytime/jsmith.ifb
END:VFREEBUSY
END:VCALENDAR
)");

const NSc::TValue EXPECTED_2 = NSc::TValue::FromJson(R"(
{
    "body" : [
        {"name" : "VERSION", "value" : "2.0"},
        {"name" : "PRODID", "value" : "-//RDU Software//NONSGML HandCal//EN"}
    ]
}
)");

constexpr TStringBuf SOURCE_3 = TStringBuf(R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//RDU Software//NONSGML HandCal//EN
BEGIN:VEVENT
DTSTART:19980313T141711Z
DTEND:19980313T141711Z
RRULE:FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR;UNTIL=19990313T141711Z
BEGIN:VALARM
TRIGGER:P0D
ACTION:AUDIO
END:VALARM
END:VEVENT
END:VCALENDAR
)");

const NSc::TValue EXPECTED_3 = NSc::TValue::FromJson(R"(
{
    "body" : [
        {"name" : "VERSION", "value" : "2.0"},
        {"name" : "PRODID", "value" : "-//RDU Software//NONSGML HandCal//EN"}
    ],
    "events" : [
        {
            "alarms" : [
                {
                    "body" : [
                        {"name" : "TRIGGER", "value" : "P0D"},
                        {"name" : "ACTION", "value" : "AUDIO"}
                    ]
                }
            ],
            "end" : {
                "time" : "1998-03-13T14:17:11",
                "timeZone" : "UTC"
            },
            "recur" : {
                "until" : "19990313T141711Z",
                "weekdays" : ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday"]
            },
            "start" : {
                "time" : "1998-03-13T14:17:11",
                "timeZone" : "UTC"
            }
        }
    ]
}
)");

NSc::TValue Parse(TStringBuf data) {
    TCalendar calendar;
    TVector<TString> lines;
    TParser().Parse(data, calendar, lines);

    NSc::TValue value;
    NVisitors::TSchemeVisitorParams params;
    params.SkipEmptyArrays = true;
    NVisitors::TSchemeDictVisitor visitor(value.GetDictMutable(), params);
    calendar.Visit(visitor);
    return value;
}

void CheckParse(TStringBuf data, NSc::TValue target) {
    NSc::TValue actual = Parse(data);
    UNIT_ASSERT(EqualJson(target, actual));
}
} // namespace

Y_UNIT_TEST_SUITE(TParserUnitTest) {
    Y_UNIT_TEST(Smoke) {
        CheckParse(SOURCE_0, EXPECTED_0);
        CheckParse(SOURCE_1, EXPECTED_1);
        CheckParse(SOURCE_2, EXPECTED_2);
        CheckParse(SOURCE_3, EXPECTED_3);
    }
}
