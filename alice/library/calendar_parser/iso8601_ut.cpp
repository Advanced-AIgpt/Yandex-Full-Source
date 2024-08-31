#include "iso8601.h"

#include <library/cpp/timezone_conversion/civil.h>
#include <library/cpp/timezone_conversion/convert.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NCalendarParser;
using namespace NDatetime;

namespace {
NDatetime::TCivilDay DesDate(TStringBuf data) {
    TStringBuf stream = data;
    const auto result = TISO8601SerDes::DesDate(stream);
    UNIT_ASSERT(stream.empty());
    return result;
}

TISO8601SerDes::TDateTime DesDateTime(TStringBuf data) {
    TStringBuf stream = data;
    const auto result = TISO8601SerDes::DesDateTime(stream);
    UNIT_ASSERT(stream.empty());
    return result;
}

Y_UNIT_TEST_SUITE(ISO8601UnitTest) {
    Y_UNIT_TEST(DesDateSmoke) {
        UNIT_ASSERT_VALUES_EQUAL(DesDate("20180102"), NDatetime::TCivilDay(2018, 1, 2));
        UNIT_ASSERT_VALUES_EQUAL(DesDate("20180331"), NDatetime::TCivilDay(2018, 3, 31));
        UNIT_ASSERT_VALUES_EQUAL(DesDate("20180430"), NDatetime::TCivilDay(2018, 4, 30));
        UNIT_ASSERT_VALUES_EQUAL(DesDate("99991231"), NDatetime::TCivilDay(9999, 12, 31));

        UNIT_ASSERT_VALUES_EQUAL(DesDate("20180228"), NDatetime::TCivilDay(2018, 2, 28));
        UNIT_ASSERT_EXCEPTION_CONTAINS(DesDate("20180229"), TISO8601SerDes::TException, "Invalid number of days: 29");
        UNIT_ASSERT_VALUES_EQUAL(DesDate("20200229"), NDatetime::TCivilDay(2020, 2, 29));

        UNIT_ASSERT_EXCEPTION_CONTAINS(DesDate("20180332"), TISO8601SerDes::TException, "Invalid number of days: 32");
        UNIT_ASSERT_EXCEPTION_CONTAINS(DesDate("20180001"), TISO8601SerDes::TException, "Invalid number of months: 0");
    }

    Y_UNIT_TEST(DesDateTimeSmoke) {
        UNIT_ASSERT_VALUES_EQUAL(DesDateTime("20180302T123015"),
                                 TISO8601SerDes::TDateTime(NDatetime::TCivilSecond(2018, 3, 2, 12, 30, 15),
                                                           TISO8601SerDes::ETimeZone::Unspecified));
        UNIT_ASSERT_VALUES_EQUAL(DesDateTime("20180302T123015Z"),
                                 TISO8601SerDes::TDateTime(NDatetime::TCivilSecond(2018, 3, 2, 12, 30, 15),
                                                           TISO8601SerDes::ETimeZone::UTC));
        UNIT_ASSERT_VALUES_EQUAL(DesDateTime("20180302T235959Z"),
                                 TISO8601SerDes::TDateTime(NDatetime::TCivilSecond(2018, 3, 2, 23, 59, 59),
                                                           TISO8601SerDes::ETimeZone::UTC));

        UNIT_ASSERT_EXCEPTION_CONTAINS(DesDateTime("20180302T241005"), TISO8601SerDes::TException,
                                       "Invalid number of hours: 24");
    }
}
} // namespace
