#include "get_date.h"

#include <alice/hollywood/library/scenarios/get_date/proto/get_date.pb.h>
#include <alice/hollywood/library/scenarios/get_date/slot_utils/calendar_utils.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/timezone_conversion/civil.h>

namespace NAlice::NHollywoodFw::NGetDate {

Y_UNIT_TEST_SUITE(GetDateUtils) {
    Y_UNIT_TEST(TestGetDateMerges) {
        // Empty vector and always false function
        TCalendarDatesArray allDates;
        allDates.CompactVector([](TCalendarDatesArray::TDateInformation&, const TCalendarDatesArray::TDateInformation&) -> bool {return false; });
        UNIT_ASSERT(allDates.Size() == 0);
        allDates.CompactVector([](TCalendarDatesArray::TDateInformation&, const TCalendarDatesArray::TDateInformation&) -> bool {return true; });
        UNIT_ASSERT(allDates.Size() == 0);
        // Full vector and always true function
        allDates.Get().push_back({*TSysDatetimeParser::Today(), TSysDatetimeParser::EDatetimeParser::Unknown, NDatetime::TCivilSecond(0)});
        allDates.Get().push_back({*TSysDatetimeParser::Today(), TSysDatetimeParser::EDatetimeParser::Unknown, NDatetime::TCivilSecond(1)});
        allDates.Get().push_back({*TSysDatetimeParser::Today(), TSysDatetimeParser::EDatetimeParser::Unknown, NDatetime::TCivilSecond(2)});
        allDates.CompactVector([](TCalendarDatesArray::TDateInformation&, const TCalendarDatesArray::TDateInformation&) -> bool {return false; });
        UNIT_ASSERT(allDates.Size() == 3);
        allDates.CompactVector([](TCalendarDatesArray::TDateInformation&, const TCalendarDatesArray::TDateInformation&) -> bool {return true; });
        UNIT_ASSERT(allDates.Size() == 1);

        allDates.Get().clear();
        allDates.Get().push_back({*TSysDatetimeParser::Today(), TSysDatetimeParser::EDatetimeParser::Unknown, NDatetime::TCivilSecond(0)});
        allDates.Get().push_back({*TSysDatetimeParser::Today(), TSysDatetimeParser::EDatetimeParser::Unknown, NDatetime::TCivilSecond(1)});
        allDates.Get().push_back({*TSysDatetimeParser::Today(), TSysDatetimeParser::EDatetimeParser::Unknown, NDatetime::TCivilSecond(0)});
        allDates.Get().push_back({*TSysDatetimeParser::Today(), TSysDatetimeParser::EDatetimeParser::Unknown, NDatetime::TCivilSecond(2)});
        allDates.Get().push_back({*TSysDatetimeParser::Today(), TSysDatetimeParser::EDatetimeParser::Unknown, NDatetime::TCivilSecond(0)});
        allDates.CompactVector([](TCalendarDatesArray::TDateInformation& p1, const TCalendarDatesArray::TDateInformation& p2) -> bool {return p1.DateResult == p2.DateResult; });
        UNIT_ASSERT(allDates.Size() == 3);
        UNIT_ASSERT(allDates.At(0).DateResult == NDatetime::TCivilSecond(0));
        UNIT_ASSERT(allDates.At(1).DateResult == NDatetime::TCivilSecond(1));
        UNIT_ASSERT(allDates.At(2).DateResult == NDatetime::TCivilSecond(2));
    }
}

} // namespace NAlice::NHollywood::NGetDate
