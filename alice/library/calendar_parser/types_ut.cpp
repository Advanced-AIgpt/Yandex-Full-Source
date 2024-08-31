#include "types.h"

#include "visitors.h"

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/timezone_conversion/convert.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>

using namespace NCalendarParser;

namespace {
constexpr TStringBuf SAMPLE_0 = "ATTENDEE;RSVP=TRUE;ROLE=REQ-PARTICIPANT:mailto:jsmith@example.com";
constexpr TStringBuf SAMPLE_1 = "RDATE;VALUE=DATE:19970304,19970504,19970704,19970904";
constexpr TStringBuf SAMPLE_2 =
              "ATTACH;FMTTYPE=text/"
              "plain;ENCODING=BASE64;VALUE=BINARY:VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZy4"sv;
constexpr TStringBuf SAMPLE_3 = "DESCRIPTION;ALTREP=\"cid:part1.0001@example.org\":The Fall'98 Wild Wizards "
                                          "Conference - - Las Vegas\\, NV\\, USA"sv;
constexpr TStringBuf SAMPLE_4 = "CATEGORIES:ANNIVERSARY,PERSONAL,SPECIAL OCCASION";
constexpr TStringBuf SAMPLE_5 =
    TStringBuf("ATTENDEE;DELEGATED-FROM=\"mailto:jsmith@example.com\":mailto:jdoe@example.com");
constexpr TStringBuf SAMPLE_6 = TStringBuf("NAME;PNAME1=\"VALUE1\",VALUE2,\"VALUE3\";PNAME2=\"VALUE4\":VALUE");
constexpr TStringBuf SAMPLE_7 =
    "NAME;PNAME1=\"значение 1\",значение 2,\"значение 3\";PNAME2=\"значение с символом , внутри\":какое-то значение"sv;

constexpr TStringBuf TARGET_0 =
              "BEGIN:VCALENDAR\r\n"
              "VERSION:2.0\r\n"
              "PRODID:-//hacksw/handcal//NONSGML v1.0//EN\r\n"

              "BEGIN:VEVENT\r\n"
              "UID:19970610T172345Z-AF23B2@example.com\r\n"
              "DTSTAMP:19970610T172345Z\r\n"
              "END:VEVENT\r\n"

              "BEGIN:VTODO\r\n"
              "DTSTAMP:19980130T134500Z\r\n"
              "ATTENDEE;PARTSTAT=ACCEPTED:mailto:jqpublic@example.com\r\n"
              "BEGIN:VALARM\r\n"
              "ACTION:AUDIO\r\n"
              "TRIGGER:19980403T120000Z\r\n"
              "ATTACH;FMTTYPE=audio/basic:http://example.com/pub/audio-files/ssbanner.aud\r\n"
              "END:VALARM\r\n"
              "END:VTODO\r\n"

              "BEGIN:VTODO\r\n"
              "NAME;P1=V1,V2;P2=V3,V4,V5;P3=\":::\":VALUE\r\n"
              "END:VTODO\r\n"

              "END:VCALENDAR\r\n"sv;

TString ToDebugString(const TContentLine& cl) {
    NSc::TValue value;
    NVisitors::TSchemeValueVisitor visitor(value, NVisitors::TSchemeVisitorParams{});
    visitor(cl, "TContentLine");
    return value.ToJsonPretty();
}

void AssertContentLinesEqual(const TContentLine& expected, const TContentLine& actual) {
    UNIT_ASSERT_EQUAL_C(
        expected,
        actual,
        TStringBuilder{} << "Expected: " << ToDebugString(expected) << Endl
                         << "Actual: " << ToDebugString(actual) << Endl
    );
}

Y_UNIT_TEST_SUITE(TTypesUnitTest_Parse) {
    Y_UNIT_TEST(Sample0) {
        TVector<TParam> params;
        params.emplace_back("RSVP", TVector<TStringBuf>{"TRUE"});
        params.emplace_back("ROLE", TVector<TStringBuf>{"REQ-PARTICIPANT"});
        const TContentLine expected("ATTENDEE", "mailto:jsmith@example.com", params);

        AssertContentLinesEqual(expected, TContentLine::Parse(SAMPLE_0));
    }

    Y_UNIT_TEST(Sample1) {
        TVector<TParam> params;
        params.emplace_back("VALUE", TVector<TStringBuf>{"DATE"});
        const TContentLine expected("RDATE", "19970304,19970504,19970704,19970904", params);

        AssertContentLinesEqual(expected, TContentLine::Parse(SAMPLE_1));
    }

    Y_UNIT_TEST(Sample2) {
        TVector<TParam> params;
        params.emplace_back("FMTTYPE", TVector<TStringBuf>{"text/plain"});
        params.emplace_back("ENCODING", TVector<TStringBuf>{"BASE64"});
        params.emplace_back("VALUE", TVector<TStringBuf>{"BINARY"});
        const TContentLine expected("ATTACH", "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZy4", params);

        AssertContentLinesEqual(expected, TContentLine::Parse(SAMPLE_2));
    }

    Y_UNIT_TEST(Sample3) {
        TVector<TParam> params;
        params.emplace_back("ALTREP", TVector<TStringBuf>{"cid:part1.0001@example.org"});
        const TContentLine expected("DESCRIPTION", "The Fall'98 Wild Wizards Conference - - Las Vegas\\, NV\\, USA",
                                    params);
        AssertContentLinesEqual(expected, TContentLine::Parse(SAMPLE_3));
    }

    Y_UNIT_TEST(Sample4) {
        const TContentLine expected("CATEGORIES", "ANNIVERSARY,PERSONAL,SPECIAL OCCASION", TVector<TParam>());
        AssertContentLinesEqual(expected, TContentLine::Parse(SAMPLE_4));
    }

    Y_UNIT_TEST(Sample5) {
        TVector<TParam> params;
        params.emplace_back("DELEGATED-FROM", TVector<TStringBuf>{"mailto:jsmith@example.com"});
        const TContentLine expected("ATTENDEE", "mailto:jdoe@example.com", params);
        AssertContentLinesEqual(expected, TContentLine::Parse(SAMPLE_5));
    }

    Y_UNIT_TEST(Sample6) {
        TVector<TParam> params;
        params.emplace_back("PNAME1", TVector<TStringBuf>{"VALUE1", "VALUE2", "VALUE3"});
        params.emplace_back("PNAME2", TVector<TStringBuf>{"VALUE4"});
        const TContentLine expected("NAME", "VALUE", params);
        AssertContentLinesEqual(expected, TContentLine::Parse(SAMPLE_6));
    }

    Y_UNIT_TEST(Sample7) {
        TVector<TParam> params;
        params.emplace_back("PNAME1", TVector<TStringBuf>{"значение 1", "значение 2", "значение 3"});
        params.emplace_back("PNAME2", TVector<TStringBuf>{"значение с символом , внутри"});
        const TContentLine expected("NAME", "какое-то значение", params);
        AssertContentLinesEqual(expected, TContentLine::Parse(SAMPLE_7));
    }
}

Y_UNIT_TEST_SUITE(TTypesUnitTest_Serialize) {
    Y_UNIT_TEST(Sample0) {
        TCalendar calendar;

        calendar.Body.emplace_back("VERSION", "2.0");
        calendar.Body.emplace_back("PRODID", "-//hacksw/handcal//NONSGML v1.0//EN");

        {
            TToDo toDo;
            toDo.Body.emplace_back("DTSTAMP", "19980130T134500Z");
            toDo.Body.emplace_back("ATTENDEE", "mailto:jqpublic@example.com",
                                   TVector<TParam>{{"PARTSTAT", TVector<TStringBuf>{"ACCEPTED"}}});

            TAlarm alarm;
            alarm.Body.emplace_back("ACTION", "AUDIO");
            alarm.Body.emplace_back("TRIGGER", "19980403T120000Z");
            alarm.Body.emplace_back("ATTACH", "http://example.com/pub/audio-files/ssbanner.aud",
                                    TVector<TParam>{{"FMTTYPE", TVector<TStringBuf>{"audio/basic"}}});

            toDo.Alarms.push_back(alarm);
            calendar.ToDos.push_back(toDo);
        }

        {
            TToDo toDo;
            toDo.Body.emplace_back("NAME", "VALUE", TVector<TParam>{{"P1", TVector<TStringBuf>{"V1", "V2"}},
                                                                    {"P2", TVector<TStringBuf>{"V3", "V4", "V5"}},
                                                                    {"P3", TVector<TStringBuf>{":::"}}});
            calendar.ToDos.push_back(toDo);
        }

        {
            TEvent event;
            event.Body.emplace_back("UID", "19970610T172345Z-AF23B2@example.com");
            event.Body.emplace_back("DTSTAMP", "19970610T172345Z");
            calendar.Events.push_back(event);
        }

        UNIT_ASSERT_STRINGS_EQUAL(TARGET_0, calendar.Serialize());
    }
}

Y_UNIT_TEST_SUITE(TypesTimeParserUnitTest) {
    Y_UNIT_TEST(Smoke) {
        {
            const TVector<TParam> params;
            UNIT_ASSERT_VALUES_EQUAL(TStart::FromContent("20180329T103020", params),
                                     TStart(NDatetime::TCivilSecond{2018, 3, 29, 10, 30, 20}));
        }

        {
            const TVector<TParam> params;
            UNIT_ASSERT_VALUES_EQUAL(
                TStart::FromContent("20180329T103020Z", params),
                TStart(NDatetime::TCivilSecond{2018, 3, 29, 10, 30, 20}, NDatetime::GetUtcTimeZone()));
        }

        {
            TVector<TParam> params;
            params.emplace_back("TZID", TVector<TStringBuf>{{"Europe/Moscow"}});
            UNIT_ASSERT_VALUES_EQUAL(
                TStart::FromContent("20180329T103020", params),
                TStart(NDatetime::TCivilSecond{2018, 3, 29, 10, 30, 20}, NDatetime::GetTimeZone("Europe/Moscow")));
        }

        {
            TVector<TParam> params;
            params.emplace_back("TZID", TVector<TStringBuf>{{"America/New_York"}});
            params.emplace_back("VALUE", TVector<TStringBuf>{{"DATE-TIME"}});
            UNIT_ASSERT_VALUES_EQUAL(
                TStart::FromContent("20180211T121314", params),
                TStart(NDatetime::TCivilSecond{2018, 2, 11, 12, 13, 14}, NDatetime::GetTimeZone("America/New_York")));
        }

        {
            TVector<TParam> params;
            params.emplace_back("TZID", TVector<TStringBuf>{{"Europe/Moscow"}});
            params.emplace_back("VALUE", TVector<TStringBuf>{{"DATE"}});
            UNIT_ASSERT_VALUES_EQUAL(
                TStart::FromContent("20170101", params),
                TStart(NDatetime::TCivilSecond{2017, 1, 1}, NDatetime::GetTimeZone("Europe/Moscow")));
        }

        {
            TVector<TParam> params;
            params.emplace_back("VALUE", TVector<TStringBuf>{{"DATE"}});
            UNIT_ASSERT_VALUES_EQUAL(TStart::FromContent("20170405", params),
                                     TStart(NDatetime::TCivilSecond{2017, 4, 5}));
        }
    }

    Y_UNIT_TEST(Negatives) {
        {
            TVector<TParam> params;
            params.emplace_back("VALUE", TVector<TStringBuf>{{"some wrong value is here"}});
            UNIT_ASSERT_EXCEPTION_CONTAINS(TStart::FromContent("20180101T010101", params), TTypesException,
                                           "Invalid VALUE value");
        }

        {
            TVector<TParam> params;
            params.emplace_back("TZID", TVector<TStringBuf>{{"Европа/Москва"}});
            UNIT_ASSERT_EXCEPTION_CONTAINS(TStart::FromContent("20180101T010101", params), NDatetime::TInvalidTimezone,
                                           "Европа/Москва");
        }

        {
            TVector<TParam> params;
            params.emplace_back("TZID", TVector<TStringBuf>{{"Europe/Moscow"}});
            UNIT_ASSERT_EXCEPTION_CONTAINS(TStart::FromContent("20180101T010101Z", params), TTypesException,
                                           "Timezones mismatch");
        }
    }
}
} // namespace
