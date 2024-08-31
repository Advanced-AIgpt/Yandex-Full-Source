#include "library/cpp/timezone_conversion/civil.h"
#include "sys_datetime.h"

#include <alice/protos/data/entities/datetime.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice {

    namespace {

        struct DateTimeCollection {
            const TStringBuf SourceValue;
            TSysDatetimeParser::EDatetimeParser ExpectedResult;
            TSysDatetimeParser::ETense Tense;
            NDatetime::TCivilSecond TimeNow;
            NDatetime::TCivilSecond TimeExpect;
        };

        static TVector<DateTimeCollection> TEST_CASES = {
            // Тесты на абсолютные даты
            {"{\"days\":12, \"months\":5}", TSysDatetimeParser::EDatetimeParser::Fixed, TSysDatetimeParser::TenseDefault, NDatetime::TCivilSecond{2020, 4, 10, 1, 2, 3}, NDatetime::TCivilSecond{2020, 5, 12, 1, 2, 3}},
            // 21 -> 2021
            {"{\"years\":21}", TSysDatetimeParser::EDatetimeParser::Fixed, TSysDatetimeParser::TenseDefault, NDatetime::TCivilSecond{2020, 4, 10, 10, 20, 33}, NDatetime::TCivilSecond{2021, 4, 10, 10, 20, 33}},
            // 99 -> 1999
            {"{\"years\":99, \"hours\":1}", TSysDatetimeParser::EDatetimeParser::Fixed, TSysDatetimeParser::TenseDefault, NDatetime::TCivilSecond{2021, 4, 10, 10, 20, 33}, NDatetime::TCivilSecond{1999, 4, 10, 1, 20, 33}},
            // 99 -> 2099 (tesne Future)
            {"{\"years\":99, \"hours\":1}", TSysDatetimeParser::EDatetimeParser::Fixed, TSysDatetimeParser::TenseFuture, NDatetime::TCivilSecond{2021, 4, 10, 10, 20, 33}, NDatetime::TCivilSecond{2099, 4, 10, 1, 20, 33}},
            // 12 -> 2112 (tesne Future)
            {"{\"years\":12, \"hours\":1}", TSysDatetimeParser::EDatetimeParser::Fixed, TSysDatetimeParser::TenseFuture, NDatetime::TCivilSecond{2021, 4, 10, 10, 20, 33}, NDatetime::TCivilSecond{2112, 4, 10, 1, 20, 33}},
            // 12 -> 2012 (tesne Past)
            {"{\"years\":12, \"hours\":1}", TSysDatetimeParser::EDatetimeParser::Fixed, TSysDatetimeParser::TensePast, NDatetime::TCivilSecond{2021, 4, 10, 10, 20, 33}, NDatetime::TCivilSecond{2012, 4, 10, 1, 20, 33}},
            // 32 -> 1932 (tesne Past)
            {"{\"years\":75, \"hours\":1}", TSysDatetimeParser::EDatetimeParser::Fixed, TSysDatetimeParser::TensePast, NDatetime::TCivilSecond{2021, 4, 10, 10, 20, 33}, NDatetime::TCivilSecond{1975, 4, 10, 1, 20, 33}},

            // Тесты на абсолютное время с/без Period (PM/PM)
            {"{\"hours\":14, \"minutes\":45, \"seconds\":33}", TSysDatetimeParser::EDatetimeParser::Fixed, TSysDatetimeParser::TenseFuture, NDatetime::TCivilSecond{2020, 11, 01, 14, 50, 34}, NDatetime::TCivilSecond{2020, 11, 01, 14, 45, 33}},
            // 8:23 AM
            {"{\"hours\":8, \"minutes\":23, \"period\":\"am\"}", TSysDatetimeParser::EDatetimeParser::Fixed, TSysDatetimeParser::TenseFuture, NDatetime::TCivilSecond{2020, 11, 01, 14, 50, 34}, NDatetime::TCivilSecond{2020, 11, 01, 8, 23, 34}},
            // 8:23 PM
            {"{\"hours\":8, \"minutes\":23, \"period\":\"pm\"}", TSysDatetimeParser::EDatetimeParser::Fixed, TSysDatetimeParser::TenseFuture, NDatetime::TCivilSecond{2020, 11, 01, 14, 50, 34}, NDatetime::TCivilSecond{2020, 11, 01, 20, 23, 34}},

            // Тесты на дни недели
            // 30/9/2021 (чт) -> 1/10/2021 (пт), Default Tense
            {"{\"weekday\":5}", TSysDatetimeParser::EDatetimeParser::FixedDayOfWeek, TSysDatetimeParser::TenseDefault, NDatetime::TCivilSecond{2021, 9, 30, 3, 2, 1}, NDatetime::TCivilSecond{2021, 10, 01, 3, 2, 1}},
            // 30/9/2021 (чт) -> 24/09/2021 (пт), Past Tense
            {"{\"weekday\":5}", TSysDatetimeParser::EDatetimeParser::FixedDayOfWeek, TSysDatetimeParser::TensePast, NDatetime::TCivilSecond{2021, 9, 30, 3, 2, 1}, NDatetime::TCivilSecond{2021, 9, 24, 3, 2, 1}},
            // 25/11/2021 (чт) -> 25/11/2021 (чт), Default Tense
            {"{\"weekday\":4}", TSysDatetimeParser::EDatetimeParser::FixedDayOfWeek, TSysDatetimeParser::TenseDefault, NDatetime::TCivilSecond{2021, 11, 25, 3, 22, 2}, NDatetime::TCivilSecond{2021, 11, 25, 3, 22, 2}},
            // 25/11/2021 (чт) -> 02/12/2021 (чт), Future Tense
            {"{\"weekday\":4}", TSysDatetimeParser::EDatetimeParser::FixedDayOfWeek, TSysDatetimeParser::TenseFuture, NDatetime::TCivilSecond{2021, 11, 25, 5, 22, 2}, NDatetime::TCivilSecond{2021, 12, 02, 5, 22, 2}},

            // Тесты на относительные даты
            // Future
            // 14:50:34 + 02:20:01 -> 17:10:35
            {"{\"hours_relative\":true, \"hours\":2, \"minutes_relative\":true, \"minutes\":20, \"seconds_relative\":true, \"seconds\":1}", TSysDatetimeParser::EDatetimeParser::RelativeFuture, TSysDatetimeParser::TenseFuture, NDatetime::TCivilSecond{2020, 11, 01, 14, 50, 34}, NDatetime::TCivilSecond{2020, 11, 01, 17, 10, 35}},
            // 01/01/2021 + (-1month) -> 01/02/2021 (always future)
            {"{\"months_relative\":true, \"months\":-1}", TSysDatetimeParser::EDatetimeParser::RelativePast, TSysDatetimeParser::TenseFuture, NDatetime::TCivilSecond{2021, 1, 1, 14, 50, 34}, NDatetime::TCivilSecond{2021, 2, 1, 14, 50, 34}},
            // 01/01/2021 + (-1month) -> 01/12/2020 (default tense == past)
            {"{\"months_relative\":true, \"months\":-1}", TSysDatetimeParser::EDatetimeParser::RelativePast, TSysDatetimeParser::TenseDefault, NDatetime::TCivilSecond{2021, 1, 1, 14, 50, 34}, NDatetime::TCivilSecond{2020, 12, 1, 14, 50, 34}},
            // 01/09/2021 + (5month) -> 01/02/2022 (future)
            {"{\"months_relative\":true, \"months\":5}", TSysDatetimeParser::EDatetimeParser::RelativeFuture, TSysDatetimeParser::TenseDefault, NDatetime::TCivilSecond{2021, 9, 1, 14, 50, 34}, NDatetime::TCivilSecond{2022, 2, 1, 14, 50, 34}},
            // Past
            // 06/11/2020 - 5d -> 01/11/2020
            {"{\"days_relative\":true, \"days\":-5}", TSysDatetimeParser::EDatetimeParser::RelativePast, TSysDatetimeParser::TenseDefault, NDatetime::TCivilSecond{2020, 11, 6, 17, 10, 34}, NDatetime::TCivilSecond{2020, 11, 01, 17, 10, 34}},
            // 01/11/2020 - 5d -> 27/10/2020
            {"{\"days_relative\":true, \"days\":-5}", TSysDatetimeParser::EDatetimeParser::RelativePast, TSysDatetimeParser::TenseDefault, NDatetime::TCivilSecond{2020, 11, 1, 17, 10, 34}, NDatetime::TCivilSecond{2020, 10, 27, 17, 10, 34}},
            // Сдвижка на большое количество часов (409 часов = 17 суток и 1 час)
            {"{\"hours_relative\":true, \"hours\":409, \"seconds_relative\":true, \"seconds\":-1}", TSysDatetimeParser::EDatetimeParser::RelativeMix, TSysDatetimeParser::TenseDefault, NDatetime::TCivilSecond{2021, 11, 01, 14, 50, 00}, NDatetime::TCivilSecond{2021, 11, 18, 15, 49, 59}},
            // Тест на "Следующий вторник"
            {"{\"weekday\":2,\"weeks\":1,\"weeks_relative\":true}", TSysDatetimeParser::EDatetimeParser::Mix, TSysDatetimeParser::TenseFuture, NDatetime::TCivilSecond{2021, 10, 14, 0, 0, 0}, NDatetime::TCivilSecond{2021, 10, 19, 0, 0, 0}},
            {"{\"weekday\":2,\"weeks\":1,\"weeks_relative\":true}", TSysDatetimeParser::EDatetimeParser::Mix, TSysDatetimeParser::TenseDefault, NDatetime::TCivilSecond{2021, 10, 14, 0, 0, 0}, NDatetime::TCivilSecond{2021, 10, 19, 0, 0, 0}},
            // Тест на "Следующий вторник" при условии, что он еще ненаступил (вторник: 11->12, следующий вторник: 11-19)
            {"{\"weekday\":2}", TSysDatetimeParser::EDatetimeParser::FixedDayOfWeek, TSysDatetimeParser::TenseFuture, NDatetime::TCivilSecond{2021, 10, 11, 0, 0, 0}, NDatetime::TCivilSecond{2021, 10, 12, 0, 0, 0}},
            {"{\"weekday\":2,\"weeks\":1,\"weeks_relative\":true}", TSysDatetimeParser::EDatetimeParser::Mix, TSysDatetimeParser::TenseFuture, NDatetime::TCivilSecond{2021, 10, 11, 0, 0, 0}, NDatetime::TCivilSecond{2021, 10, 19, 0, 0, 0}},
            {"{\"weekday\":2,\"weeks\":1,\"weeks_relative\":true}", TSysDatetimeParser::EDatetimeParser::Mix, TSysDatetimeParser::TenseDefault, NDatetime::TCivilSecond{2021, 10, 11, 0, 0, 0}, NDatetime::TCivilSecond{2021, 10, 19, 0, 0, 0}}
        };
    }

    Y_UNIT_TEST_SUITE(SysDateTime) {

        Y_UNIT_TEST(SysDateSimple1) {
            TMaybe<TSysDatetimeParser> parser = TSysDatetimeParser::Parse("{}");
            UNIT_ASSERT(parser.Empty());
        }
        Y_UNIT_TEST(SysDateSimple2) {
            TSysDateValue proto;
            TMaybe<TSysDatetimeParser> parser = TSysDatetimeParser::Parse(proto);
            UNIT_ASSERT(parser.Empty());
        }
        Y_UNIT_TEST(SysDateSimple3) {
            TSysTimeValue proto;
            TMaybe<TSysDatetimeParser> parser = TSysDatetimeParser::Parse(proto);
            UNIT_ASSERT(parser.Empty());
        }
        Y_UNIT_TEST(SysDateSimple4) {
            TSysDatetimeValue proto;
            TMaybe<TSysDatetimeParser> parser = TSysDatetimeParser::Parse(proto);
            UNIT_ASSERT(parser.Empty());
        }

        Y_UNIT_TEST(SysDateAbsolute1) {

            TMaybe<TSysDatetimeParser> parser = TSysDatetimeParser::Parse("{\"years\":2020, \"months\":8, \"days\":21}");

            UNIT_ASSERT(parser.Defined());
            UNIT_ASSERT_EQUAL(parser->GetParseInfo(), TSysDatetimeParser::EDatetimeParser::Fixed);
            UNIT_ASSERT_EQUAL(parser->GetParseContent(), TSysDatetimeParser::EDatetimeContent::DateOnly);
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Year.Get(), 2020);
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Month.Get(), 8);
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Day.Get(), 21);
            UNIT_ASSERT(parser->GetRawDatetime().DayOfWeek.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Hour.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Minutes.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Seconds.IsNotFilled());
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Period, TSysDatetimeParser::TParseDateTime::PeriodNone);
        }

        Y_UNIT_TEST(SysDateAbsolute2) {

            TMaybe<TSysDatetimeParser> parser = TSysDatetimeParser::Parse("{\"hours\":11, \"minutes\":34, \"seconds\":56}");

            UNIT_ASSERT(parser.Defined());
            UNIT_ASSERT_EQUAL(parser->GetParseInfo(), TSysDatetimeParser::EDatetimeParser::Fixed);
            UNIT_ASSERT_EQUAL(parser->GetParseContent(), TSysDatetimeParser::EDatetimeContent::TimeOnly);
            UNIT_ASSERT(parser->GetRawDatetime().Year.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Month.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Day.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().DayOfWeek.IsNotFilled());
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Hour.Get(), 11);
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Minutes.Get(), 34);
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Seconds.Get(), 56);
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Period, TSysDatetimeParser::TParseDateTime::PeriodNone);
        }

        Y_UNIT_TEST(SysDateAbsolute3) {

            TMaybe<TSysDatetimeParser> parser = TSysDatetimeParser::Parse("{\"hours\":5, \"period\":\"pm\", \"minutes\":0, \"seconds\":29}");

            UNIT_ASSERT(parser.Defined());
            UNIT_ASSERT_EQUAL(parser->GetParseInfo(), TSysDatetimeParser::EDatetimeParser::Fixed);
            UNIT_ASSERT_EQUAL(parser->GetParseContent(), TSysDatetimeParser::EDatetimeContent::TimeOnly);
            UNIT_ASSERT(parser->GetRawDatetime().Year.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Month.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Day.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().DayOfWeek.IsNotFilled());
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Hour.Get(), 5);
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Minutes.Get(), 0);
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Seconds.Get(), 29);
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Period, TSysDatetimeParser::TParseDateTime::PeriodPM);
        }

        Y_UNIT_TEST(SysDateDayOfWeek1) {

            TMaybe<TSysDatetimeParser> parser = TSysDatetimeParser::Parse("{\"weekday\":2}");

            UNIT_ASSERT(parser.Defined());
            UNIT_ASSERT_EQUAL(parser->GetParseInfo(), TSysDatetimeParser::EDatetimeParser::FixedDayOfWeek);
            UNIT_ASSERT_EQUAL(parser->GetParseContent(), TSysDatetimeParser::EDatetimeContent::DateOnly);
            UNIT_ASSERT(parser->GetRawDatetime().Year.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Month.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Day.IsNotFilled());
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().DayOfWeek.Get(), 2);
            UNIT_ASSERT(parser->GetRawDatetime().DayOfWeek.IsAbsolute());
            UNIT_ASSERT(parser->GetRawDatetime().Hour.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Minutes.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Seconds.IsNotFilled());
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Period, TSysDatetimeParser::TParseDateTime::PeriodNone);
        }

        Y_UNIT_TEST(SysDateRelative1) {

            TMaybe<TSysDatetimeParser> parser = TSysDatetimeParser::Parse("{\"years_relative\":true, \"years\":-10, \"months_relative\":true, \"months\":5}");

            UNIT_ASSERT(parser.Defined());
            UNIT_ASSERT_EQUAL(parser->GetParseInfo(), TSysDatetimeParser::EDatetimeParser::RelativeMix);
            UNIT_ASSERT_EQUAL(parser->GetParseContent(), TSysDatetimeParser::EDatetimeContent::DateOnly);

            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Year.Get(), -10);
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Month.Get(), 5);
            UNIT_ASSERT(parser->GetRawDatetime().Year.IsRelative());
            UNIT_ASSERT(parser->GetRawDatetime().Month.IsRelative());
            UNIT_ASSERT(parser->GetRawDatetime().Day.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().DayOfWeek.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Hour.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Minutes.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Seconds.IsNotFilled());
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Period, TSysDatetimeParser::TParseDateTime::PeriodNone);
        }

        Y_UNIT_TEST(SysTimeRelative1) {

            TMaybe<TSysDatetimeParser> parser = TSysDatetimeParser::Parse("{\"hours_relative\":true, \"hours\":-1, \"seconds_relative\":true, \"seconds\":-1}");

            UNIT_ASSERT(parser.Defined());
            UNIT_ASSERT_EQUAL(parser->GetParseInfo(), TSysDatetimeParser::EDatetimeParser::RelativePast);
            UNIT_ASSERT_EQUAL(parser->GetParseContent(), TSysDatetimeParser::EDatetimeContent::TimeOnly);

            UNIT_ASSERT(parser->GetRawDatetime().Year.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Month.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Year.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Month.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Day.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().DayOfWeek.IsNotFilled());
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Hour.Get(), -1);
            UNIT_ASSERT(parser->GetRawDatetime().Minutes.IsNotFilled());
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Seconds.Get(), -1);
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Period, TSysDatetimeParser::TParseDateTime::PeriodNone);
        }

        Y_UNIT_TEST(SysTimeAbsolute1) {

            TMaybe<TSysDatetimeParser> parser = TSysDatetimeParser::Parse("{\"days\":40, \"minutes\":112, \"period\":\"pm\"}");

            UNIT_ASSERT(parser.Defined());
            UNIT_ASSERT_EQUAL(parser->GetParseInfo(), TSysDatetimeParser::EDatetimeParser::Fixed);
            UNIT_ASSERT_EQUAL(parser->GetParseContent(), TSysDatetimeParser::EDatetimeContent::DateTime);

            UNIT_ASSERT(parser->GetRawDatetime().Year.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Month.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Year.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Month.IsNotFilled());
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Day.Get(), 40);
            UNIT_ASSERT(parser->GetRawDatetime().Day.IsAbsolute());
            UNIT_ASSERT(parser->GetRawDatetime().DayOfWeek.IsNotFilled());
            UNIT_ASSERT(parser->GetRawDatetime().Hour.IsNotFilled());
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Minutes.Get(), 112);
            UNIT_ASSERT(parser->GetRawDatetime().Minutes.IsAbsolute());
            UNIT_ASSERT(parser->GetRawDatetime().Seconds.IsNotFilled());
            UNIT_ASSERT_EQUAL(parser->GetRawDatetime().Period, TSysDatetimeParser::TParseDateTime::PeriodPM);
        }

        Y_UNIT_TEST(SysProto1) {
            TSysDateValue proto;

            proto.SetDays(10);
            proto.SetMonths(2);
            proto.SetYears(1);

            TMaybe<TSysDatetimeParser> parser1 = TSysDatetimeParser::Parse(proto);
            UNIT_ASSERT(parser1.Defined());
            UNIT_ASSERT_EQUAL(parser1->GetParseInfo(), TSysDatetimeParser::EDatetimeParser::Fixed);
            UNIT_ASSERT_EQUAL(parser1->GetParseContent(), TSysDatetimeParser::EDatetimeContent::DateOnly);
            UNIT_ASSERT_EQUAL(parser1->GetRawDatetime().Year.Get(), 1);
            UNIT_ASSERT(parser1->GetRawDatetime().Year.IsAbsolute());
            UNIT_ASSERT_EQUAL(parser1->GetRawDatetime().Month.Get(), 2);
            UNIT_ASSERT(parser1->GetRawDatetime().Month.IsAbsolute());
            UNIT_ASSERT_EQUAL(parser1->GetRawDatetime().Day.Get(), 10);
            UNIT_ASSERT(parser1->GetRawDatetime().Day.IsAbsolute());
            UNIT_ASSERT(parser1->GetRawDatetime().DayOfWeek.IsNotFilled());
            UNIT_ASSERT(parser1->GetRawDatetime().Hour.IsNotFilled());
            UNIT_ASSERT(parser1->GetRawDatetime().Minutes.IsNotFilled());
            UNIT_ASSERT(parser1->GetRawDatetime().Seconds.IsNotFilled());
            UNIT_ASSERT_EQUAL(parser1->GetRawDatetime().Period, TSysDatetimeParser::TParseDateTime::PeriodNone);

            proto.SetDaysRelative(false);
            proto.SetMonthsRelative(false);
            proto.SetYearsRelative(false);

            TMaybe<TSysDatetimeParser>  parser2 = TSysDatetimeParser::Parse(proto);
            UNIT_ASSERT(parser2.Defined());
            UNIT_ASSERT_EQUAL(parser2->GetParseInfo(), TSysDatetimeParser::EDatetimeParser::Fixed);
            UNIT_ASSERT_EQUAL(parser2->GetParseContent(), TSysDatetimeParser::EDatetimeContent::DateOnly);
            UNIT_ASSERT_EQUAL(parser2->GetRawDatetime().Year.Get(), 1);
            UNIT_ASSERT(parser2->GetRawDatetime().Year.IsAbsolute());
            UNIT_ASSERT_EQUAL(parser2->GetRawDatetime().Month.Get(), 2);
            UNIT_ASSERT(parser2->GetRawDatetime().Month.IsAbsolute());
            UNIT_ASSERT_EQUAL(parser2->GetRawDatetime().Day.Get(), 10);
            UNIT_ASSERT(parser2->GetRawDatetime().Day.IsAbsolute());
            UNIT_ASSERT(parser2->GetRawDatetime().DayOfWeek.IsNotFilled());
            UNIT_ASSERT(parser2->GetRawDatetime().Hour.IsNotFilled());
            UNIT_ASSERT(parser2->GetRawDatetime().Minutes.IsNotFilled());
            UNIT_ASSERT(parser2->GetRawDatetime().Seconds.IsNotFilled());
            UNIT_ASSERT_EQUAL(parser2->GetRawDatetime().Period, TSysDatetimeParser::TParseDateTime::PeriodNone);

            proto.SetDaysRelative(true);
            proto.SetMonthsRelative(true);
            proto.SetYearsRelative(true);

            TMaybe<TSysDatetimeParser> parser3 = TSysDatetimeParser::Parse(proto);
            UNIT_ASSERT(parser3.Defined());
            UNIT_ASSERT_EQUAL(parser3->GetParseInfo(), TSysDatetimeParser::EDatetimeParser::RelativeFuture);
            UNIT_ASSERT_EQUAL(parser3->GetParseContent(), TSysDatetimeParser::EDatetimeContent::DateOnly);
            UNIT_ASSERT_EQUAL(parser3->GetRawDatetime().Year.Get(), 1);
            UNIT_ASSERT(parser3->GetRawDatetime().Year.IsRelative());
            UNIT_ASSERT_EQUAL(parser3->GetRawDatetime().Month.Get(), 2);
            UNIT_ASSERT(parser3->GetRawDatetime().Month.IsRelative());
            UNIT_ASSERT_EQUAL(parser3->GetRawDatetime().Day.Get(), 10);
            UNIT_ASSERT(parser3->GetRawDatetime().Day.IsRelative());
            UNIT_ASSERT(parser3->GetRawDatetime().DayOfWeek.IsNotFilled());
            UNIT_ASSERT(parser3->GetRawDatetime().Hour.IsNotFilled());
            UNIT_ASSERT(parser3->GetRawDatetime().Minutes.IsNotFilled());
            UNIT_ASSERT(parser3->GetRawDatetime().Seconds.IsNotFilled());
            UNIT_ASSERT_EQUAL(parser3->GetRawDatetime().Period, TSysDatetimeParser::TParseDateTime::PeriodNone);
        }

        Y_UNIT_TEST(SysProto2) {
            TSysTimeValue proto;

            proto.SetSeconds(55);
            proto.SetMinutes(10);
            proto.SetHours(4);

            TMaybe<TSysDatetimeParser> parser1 = TSysDatetimeParser::Parse(proto);
            UNIT_ASSERT(parser1.Defined());
            UNIT_ASSERT_EQUAL(parser1->GetParseInfo(), TSysDatetimeParser::EDatetimeParser::Fixed);
            UNIT_ASSERT_EQUAL(parser1->GetParseContent(), TSysDatetimeParser::EDatetimeContent::TimeOnly);
            UNIT_ASSERT(parser1->GetRawDatetime().Year.IsNotFilled());
            UNIT_ASSERT(parser1->GetRawDatetime().Month.IsNotFilled());
            UNIT_ASSERT(parser1->GetRawDatetime().Day.IsNotFilled());
            UNIT_ASSERT(parser1->GetRawDatetime().DayOfWeek.IsNotFilled());
            UNIT_ASSERT(parser1->GetRawDatetime().Hour.IsAbsolute());
            UNIT_ASSERT_EQUAL(parser1->GetRawDatetime().Hour.Get(), 4);
            UNIT_ASSERT(parser1->GetRawDatetime().Minutes.IsAbsolute());
            UNIT_ASSERT_EQUAL(parser1->GetRawDatetime().Minutes.Get(), 10);
            UNIT_ASSERT(parser1->GetRawDatetime().Seconds.IsAbsolute());
            UNIT_ASSERT_EQUAL(parser1->GetRawDatetime().Seconds.Get(), 55);
            UNIT_ASSERT_EQUAL(parser1->GetRawDatetime().Period, TSysDatetimeParser::TParseDateTime::PeriodNone);

            proto.SetSecondsRelative(false);
            proto.SetMinutesRelative(false);
            proto.SetHoursRelative(false);

            TMaybe<TSysDatetimeParser> parser2 = TSysDatetimeParser::Parse(proto);
            UNIT_ASSERT(parser2.Defined());
            UNIT_ASSERT_EQUAL(parser2->GetParseInfo(), TSysDatetimeParser::EDatetimeParser::Fixed);
            UNIT_ASSERT_EQUAL(parser2->GetParseContent(), TSysDatetimeParser::EDatetimeContent::TimeOnly);
            UNIT_ASSERT(parser2->GetRawDatetime().Year.IsNotFilled());
            UNIT_ASSERT(parser2->GetRawDatetime().Month.IsNotFilled());
            UNIT_ASSERT(parser2->GetRawDatetime().Day.IsNotFilled());
            UNIT_ASSERT(parser2->GetRawDatetime().DayOfWeek.IsNotFilled());
            UNIT_ASSERT(parser2->GetRawDatetime().Hour.IsAbsolute());
            UNIT_ASSERT_EQUAL(parser2->GetRawDatetime().Hour.Get(), 4);
            UNIT_ASSERT(parser2->GetRawDatetime().Minutes.IsAbsolute());
            UNIT_ASSERT_EQUAL(parser2->GetRawDatetime().Minutes.Get(), 10);
            UNIT_ASSERT(parser2->GetRawDatetime().Seconds.IsAbsolute());
            UNIT_ASSERT_EQUAL(parser2->GetRawDatetime().Seconds.Get(), 55);
            UNIT_ASSERT_EQUAL(parser2->GetRawDatetime().Period, TSysDatetimeParser::TParseDateTime::PeriodNone);

            proto.SetSeconds(-55);
            proto.SetSecondsRelative(true);
            proto.SetMinutesRelative(true);
            proto.SetHoursRelative(true);
            proto.SetPeriod("am");

            TMaybe<TSysDatetimeParser> parser3 = TSysDatetimeParser::Parse(proto);
            UNIT_ASSERT(parser3.Defined());
            UNIT_ASSERT_EQUAL(parser3->GetParseInfo(), TSysDatetimeParser::EDatetimeParser::RelativeMix);
            UNIT_ASSERT_EQUAL(parser3->GetParseContent(), TSysDatetimeParser::EDatetimeContent::TimeOnly);
            UNIT_ASSERT(parser3->GetRawDatetime().Year.IsNotFilled());
            UNIT_ASSERT(parser3->GetRawDatetime().Month.IsNotFilled());
            UNIT_ASSERT(parser3->GetRawDatetime().Day.IsNotFilled());
            UNIT_ASSERT(parser3->GetRawDatetime().DayOfWeek.IsNotFilled());
            UNIT_ASSERT(parser3->GetRawDatetime().Hour.IsRelative());
            UNIT_ASSERT_EQUAL(parser3->GetRawDatetime().Hour.Get(), 4);
            UNIT_ASSERT(parser3->GetRawDatetime().Minutes.IsRelative());
            UNIT_ASSERT_EQUAL(parser3->GetRawDatetime().Minutes.Get(), 10);
            UNIT_ASSERT(parser3->GetRawDatetime().Seconds.IsRelative());
            UNIT_ASSERT_EQUAL(parser3->GetRawDatetime().Seconds.Get(), -55);
            UNIT_ASSERT_EQUAL(parser3->GetRawDatetime().Period, TSysDatetimeParser::TParseDateTime::PeriodAM);
        }

        Y_UNIT_TEST(SysDateSpecial) {
            TMaybe<TSysDatetimeParser> today = TSysDatetimeParser::Today();
            TMaybe<TSysDatetimeParser> yesterday = TSysDatetimeParser::Yesterday();
            TMaybe<TSysDatetimeParser> tomorrow = TSysDatetimeParser::Tomorrow();

            NDatetime::TCivilSecond timeNow = NDatetime::TCivilSecond{2021, 4, 10, 1, 2, 3};
            NDatetime::TCivilSecond timeNew;

            timeNew = yesterday->GetTargetDateTime(timeNow, TSysDatetimeParser::ETense::TenseDefault);
            timeNew = today->GetTargetDateTime(timeNew, TSysDatetimeParser::ETense::TenseDefault);
            timeNew = tomorrow->GetTargetDateTime(timeNew, TSysDatetimeParser::ETense::TenseDefault);

            // Yesterday + Today + Tomorrow == Now
            UNIT_ASSERT_EQUAL(timeNew, timeNow);
        }

        Y_UNIT_TEST(SysDateAll) {

            for (const auto& it : TEST_CASES) {
                TMaybe<TSysDatetimeParser> parser = TSysDatetimeParser::Parse(it.SourceValue.Data());

                UNIT_ASSERT(parser.Defined());
                UNIT_ASSERT_EQUAL(parser->GetParseInfo(), it.ExpectedResult);

                NDatetime::TCivilSecond res = parser->GetTargetDateTime(it.TimeNow, it.Tense);
                UNIT_ASSERT(res == it.TimeExpect);

                // Convert to proto / json and back
                TMaybe<TSysDatetimeParser> parser2 = TSysDatetimeParser::ParseJson(parser->GetAsJsonDatetime());
                TMaybe<TSysDatetimeParser> parser3 = TSysDatetimeParser::Parse(parser->GetAsProtoDatetime());

                UNIT_ASSERT(parser2.Defined());
                UNIT_ASSERT(parser3.Defined());
                UNIT_ASSERT(parser->GetRawDatetime() == parser2->GetRawDatetime());
                UNIT_ASSERT(parser->GetRawDatetime() == parser3->GetRawDatetime());
            }
        }

        Y_UNIT_TEST(YearCorrection) {
            TSysDateValue proto;
            proto.SetYears(22);
            TMaybe<TSysDatetimeParser> parser = TSysDatetimeParser::Parse(proto);
            UNIT_ASSERT(parser.Defined());

            const NDatetime::TCivilSecond timeNow = NDatetime::TCivilSecond{2021, 4, 10, 1, 2, 3};
            NDatetime::TCivilSecond res1 = parser->GetTargetDateTime(timeNow, TSysDatetimeParser::ETense::TenseDefault);
            NDatetime::TCivilSecond res2 = parser->GetTargetDateTime(timeNow, TSysDatetimeParser::ETense::TenseFuture);
            NDatetime::TCivilSecond res3 = parser->GetTargetDateTime(timeNow, TSysDatetimeParser::ETense::TensePast);

            UNIT_ASSERT(res1.year() == 2022);
            UNIT_ASSERT(res2.year() == 2022);
            UNIT_ASSERT(res3.year() == 1922);

            proto.SetYears(68); // More than 40 years forward
            parser = TSysDatetimeParser::Parse(proto);
            res1 = parser->GetTargetDateTime(timeNow, TSysDatetimeParser::ETense::TenseDefault);
            res2 = parser->GetTargetDateTime(timeNow, TSysDatetimeParser::ETense::TenseFuture);
            res3 = parser->GetTargetDateTime(timeNow, TSysDatetimeParser::ETense::TensePast);

            UNIT_ASSERT(res1.year() == 1968);
            UNIT_ASSERT(res2.year() == 2068);
            UNIT_ASSERT(res3.year() == 1968);

            // Change default year correction
            proto.SetYears(25);
            parser = TSysDatetimeParser::Parse(proto);
            parser->SetDefautYearCorrection(1);
            res1 = parser->GetTargetDateTime(timeNow, TSysDatetimeParser::ETense::TenseDefault);
            res2 = parser->GetTargetDateTime(timeNow, TSysDatetimeParser::ETense::TenseFuture);
            res3 = parser->GetTargetDateTime(timeNow, TSysDatetimeParser::ETense::TensePast);

            UNIT_ASSERT(res1.year() == 1925);
            UNIT_ASSERT(res2.year() == 2025);
            UNIT_ASSERT(res3.year() == 1925);
        }

        Y_UNIT_TEST(HumanHoursCorrection) {
            TSysDateValue proto;

            proto.SetDays(1);
            proto.SetDaysRelative(true); // Tomorrow
            TMaybe<TSysDatetimeParser> parser = TSysDatetimeParser::Parse(proto);
            UNIT_ASSERT(parser.Defined());

            const NDatetime::TCivilSecond timeNow = NDatetime::TCivilSecond{2022, 1, 20, 1, 25, 0};
            NDatetime::TCivilSecond res = parser->GetTargetDateTime(timeNow, TSysDatetimeParser::ETense::TenseDefault);
            UNIT_ASSERT(res.day() == 21); // Go to tomorrow

            parser->SetHumanCorrection(TDuration::Minutes(20));
            res = parser->GetTargetDateTime(timeNow, TSysDatetimeParser::ETense::TenseDefault);
            UNIT_ASSERT(res.day() == 21); // Go to tomorrow, because timeNow (1:25) is more than correction (20m)

            parser->SetHumanCorrection(TDuration::Minutes(60));
            res = parser->GetTargetDateTime(timeNow, TSysDatetimeParser::ETense::TenseDefault);
            UNIT_ASSERT(res.day() == 21); // Go to tomorrow, because timeNow (1:25) is more than correction (20m)

            parser->SetHumanCorrection(TDuration::Hours(2));
            res = parser->GetTargetDateTime(timeNow, TSysDatetimeParser::ETense::TenseDefault);
            UNIT_ASSERT(res.day() == 20); // Remains today, because timeNow (1:25) is less than correction (2h)
        }
    }

} // namespace NAlice
