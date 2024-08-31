#include "common.h"

#include <alice/nlu/libs/fst/fst_date_time.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NTestHelpers;

Y_UNIT_TEST_SUITE(TFstDateTimeRuTests) {

    static const TTestCaseRunner<TFstDateTime>& T() {
        static const TTestCaseRunner<TFstDateTime> T{"DATETIME", LANG_RUS};
        return T;
    }

    static TEntity CE(size_t begin, size_t end, TStringBuf json) {
        return CreateEntity(begin, end, "DATETIME", NSc::TValue::FromJsonThrow(json));
    }

    Y_UNIT_TEST(ParseEmpty) {
        const TTestCase testCases[] = {
            {"", {}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeHalfPastFour) {
        const TTestCase testCases[] = {
            {"половина пятого вечера", {CE(0, 3, R"({"hours": 16, "minutes": 30})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(RelativeDate) {
        const TTestCase testCases[] = {
            {"через год и три с половиной месяца",
             {CE(0, 5,
                 R"({"years": 1,
                     "years_relative": true,
                     "months": 3,
                     "months_relative": true,
                     "weeks": 2,
                     "weeks_relative": true})")}},
            {"через два года", {CE(0, 3, R"({"years": 2, "years_relative": true})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(RelativeTime) {
        const TTestCase testCases[] = {
            {"через 3 часа, 9 минут, 15 секунд приедет поезд",
             {CE(0, 7,
                 R"({"time_relative": true,
                     "hours": 3,
                     "minutes": 9,
                     "seconds": 15}
                   )")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeRelativeDay) {
        const TTestCase testCases[] = {
            {"машину на без 15 6 завтра",
             {CE(2, 6,
                 R"({"hours": 5,
                     "days_relative": true,
                     "minutes": 45,
                     "days": 1}
                   )")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeWithLeftRightContext) {
        const TTestCase testCases[] = {
            {"встретимся послезавтра в без 15 6 вечера",
             {CE(1, 7,
                 R"({"days": 2,
                     "days_relative": true,
                     "hours": 17,
                     "minutes": 45}
                   )")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeWithLeftRightContextUnnorm) {
        const TTestCase testCases[] = {
            {"встретимся послезавтра, в без пятнадцати шесть вечера",
             {CE(1, 7,
                 R"({"days": 2,
                     "days_relative": true,
                     "hours": 17,
                     "minutes": 45}
                   )")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeContextWithPrep) {
        const TTestCase testCases[] = {
            {"встретимся послезавтра приблизительно в 17 10",
             {CE(1, 6,
                 R"({"days": 2,
                     "days_relative": true,
                     "hours": 17,
                     "minutes": 10}
                   )")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeContextWithPrepUnnorm) {
        const TTestCase testCases[] = {
            {"встретимся послезавтра, около десяти минут шестого вечера!",
             {CE(1, 7,
                 R"({"days": 2,
                     "days_relative": true,
                     "hours": 17,
                     "minutes": 10}
                   )")}},
            {"ноль минут ноль секунд", {}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(DateAndRelativeDate) {
        const TTestCase testCases[] = {
            {"праздник победы 9 мая 1945 го года был 2 недели назад",
             {CE(2, 7,
                 R"({"months": 5,
                     "days": 9,
                     "years": 1945}
                   )"),
              CE(8, 11, R"({"weeks": -2, "weeks_relative": true})")}},
            {"пару лет назад", {CE(0, 3, R"({"years": -2, "years_relative": true})")}},
            {"через пару лет", {CE(0, 3, R"({"years": 2, "years_relative": true})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(Weekday) {
        const TTestCase testCases[] = {
            {"погода в следующее воскресенье", {CE(2, 4, R"({"weeks": 1, "weeks_relative": true, "weekday": 7})")}},
            {"дата прошлого четверга", {CE(1, 3, R"({"weeks": -1, "weeks_relative": true, "weekday": 4})")}},
            {"к этой среде", {CE(2, 3, R"({"weekday": 3})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(DateAndTimeFormattedUnnorm) {
        const TTestCase testCases[] = {
            {"08.03.2016, в 10:30 будет праздник!!",
             {CE(0, 3, R"({"hours": 10, "months": 3, "minutes": 30, "days": 8, "years": 2016})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(Date) {
        const TTestCase testCases[] = {
            {"на 5 декабря 2016", {CE(1, 4, R"({"months": 12, "days": 5, "years": 2016})")}},
            {"в ноябре 2014 года", {CE(1, 4, R"({"months": 11, "years": 2014})")}},
            {"концерт субботний вечер 23 сентября 17 года", {CE(3, 7, R"({"days": 23, "months": 9, "years": 2017})")}},
            {"программа передач на вечер 1 октября 1 канала", {CE(4, 6, R"({"months": 10, "days": 1})")}},
            {"восемьсот пятьдесят третий год москва что за день", {CE(0, 2, R"({"years": 853})")}},
// DIALOG-912
            {"обзор игрового дня 14 10 17 год", {CE(5, 7, R"({"years": 2017})")}},
            {"при какой день 5 6 301 года месяц октябрь ноябрь февраль март апрель июль август сентябрь",
             {CE(5, 7, R"({"years": 301})"), CE(8, 9, R"({"months": 10})"), CE(9, 10, R"({"months": 11})"),
              CE(10, 11, R"({"months": 2})"), CE(11, 12, R"({"months": 3})"), CE(12, 13, R"({"months": 4})"),
              CE(13, 14, R"({"months": 7})"), CE(14, 15, R"({"months": 8})"), CE(15, 16, R"({"months": 9})")}},
            {"надо амине амине идет в школу 8 год 8 5 день 21 года",
             {CE(6, 8, R"({"years": 2008})"), CE(11, 13, R"({"years": 2021})")}},
            {"внести программу в виде цифры хотя бы что там должно быть на другой барахты его 96 год в 1 3 дня и 91 4",
             {CE(15, 17, R"({"years": 1996})")}},
            {"канал россия 1 сегодня 15 октября 17 года программа канал россия 1",
             {CE(2, 8, R"({"hours": 1, "months": 10, "minutes": 0, "days": 15, "years": 2017})")}},
            {"какой день недели 1 января следующего года",
             {CE(3, 5, R"({"days": 1, "months": 1})"),
              CE(5, 7, R"({"years": 1, "years_relative": true})")}}, // not using postprocessing in this test
            {"в следующем году", {CE(1, 3, R"({"years": 1, "years_relative": true})")}},
            {"будильник на 11:30 10 марта 2018 года",
             {CE(2, 7, R"({"hours": 11, "minutes": 30, "days": 10, "months": 3, "years": 2018})")}},
            {"8 утра 24 октября", {CE(0, 4, R"({"hours": 8, "minutes": 0, "days": 24, "months": 10})")}},
            {"на 29 декабря в 7 вечера", {CE(1, 6 , R"({"months": 12, "days": 29, "hours": 19, "minutes": 0})")}},
            {"в 7 вечера на 29 декабря", {CE(1, 6, R"({"months": 12, "days": 29, "hours": 19, "minutes": 0})")}},
            // json parser
            {"вспомни седьмой год", {CE(1,3, R"({"years": 2007})")}},
            {"сороковой год", {CE(0,2, R"({"years": 2040})")}},
            {"сорок первый год", {CE(0,2, R"({"years": 1941})")}},
            {"восьмидесятый год", {CE(0,2, R"({"years": 1980})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TwoObjects) {
        const TTestCase testCases[] = {
            {"1 мая - день труда, в 3 после полудня на картошку .",
             {CE(0, 2, R"({"months": 5, "days": 1})"),
              CE(6, 9, R"({"hours": 15, "minutes": 0})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(HalfPastEvening) {
        const TTestCase testCases[] = {
            {"машину на полшестого вечера",
             {CE(2, 4, R"({"hours": 17, "minutes": 30})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(HalfPastMorning) {
        const TTestCase testCases[] = {
            {"машину на полшестого утра",
             {CE(2, 4, R"({"hours": 5, "minutes": 30})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeMorning) {
        const TTestCase testCases[] = {
            {"машину на 6 30 утра",
             {CE(2, 5, R"({"hours": 6, "minutes": 30})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeFormattedMorning) {
        const TTestCase testCases[] = {
            {"машину на 6:30 утра",
             {CE(2, 4, R"({"hours": 6, "minutes": 30})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeEveningContext) {
        const TTestCase testCases[] = {
            {"машину к вечеру на шесть сорок пять ",
             {CE(2, 6, R"({"hours": 18, "minutes": 45})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeWithWords) {
        const TTestCase testCases[] = {
            {"машину на 7 часов 30 минут",
             {CE(2, 6, R"({"hours": 7, "minutes": 30})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeMorningRelativeDay) {
        const TTestCase testCases[] = {
            {"будильник на 6 утра завтра",
             {CE(2, 5, R"({"hours": 6, "minutes": 0, "days_relative": true, "days": 1})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeEveningRelativeDayInsertedPrep) {
        const TTestCase testCases[] = {
            {"были там позавчера вечером примерно около девяти тридцати",
             {CE(2, 8, R"({"hours": 21, "days_relative": true, "minutes": 30, "days": -2})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(Noon) {
        const TTestCase testCases[] = {
            {"машину в полдень",
             {CE(2, 3, R"({"hours": 12, "minutes": 0})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(Midnight) {
        const TTestCase testCases[] = {
            {"мне нужно такси в полночь",
             {CE(4, 5, R"({"hours": 0, "minutes": 0})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(YesterdayTime) {
        const TTestCase testCases[] = {
            {"вчера в 10 минут первого утра",
             {CE(0, 6, R"({
                "hours": 0,
                "minutes": 10,
                "days": -1,
                "days_relative": true})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(WeekdayTime) {
        const TTestCase testCases[] = {
            {"в следующую субботу в без пятнадцати шесть вечера",
             {CE(1, 8, R"({"hours": 17, "minutes": 45, "weekday": 6, "weeks": 1, "weeks_relative": true})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(WeekdayTimePast) {
        const TTestCase testCases[] = {
            {"четверть восьмого утра в прошлую среду",
             {CE(0, 6, R"({"hours": 7, "weeks": -1, "minutes": 15, "weekday": 3, "weeks_relative": true})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(WeekdayTimePresent) {
        const TTestCase testCases[] = {
            {"четверть восьмого утра в среду",
             {CE(0, 5, R"({"hours": 7, "minutes": 15, "weekday": 3})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(WeekdayTimePresentCtx) {
        const TTestCase testCases[] = {
            {"четверть восьмого утра в эту среду",
             {CE(0, 3, R"({"hours": 7, "minutes": 15})"),
              CE(5, 6, R"({"weekday": 3})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(WeekdayTimePresentCtx2) {
        const TTestCase testCases[] = {
            {"в ближайшую среду на четверть восьмого утра",
             {CE(1, 7, R"({"hours": 7, "minutes": 15, "weekday": 3})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(HoursFormattedNoMins) {
        const TTestCase testCases[] = {
            {"давай лучше в 20:00",
             {CE(3, 4, R"({"hours": 20, "minutes": 0})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TwentyHoursByWords) {
        // xfail HH MM format only supported within time context
        const TTestCase testCases[] = {
            {"в двадцать ноль ноль", {}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(HoursMinsUtt) {
        // xfail HH MM format only supported within time context
        const TTestCase testCases[] = {
            {"в двадцать один тридцать два", {}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(SpecialHoursMins) {
        const TTestCase testCases[] = {
            {"через полчаса", {CE(0, 2, R"({"minutes": 30, "minutes_relative": true})")}},
            {"через полтора часа",
             {CE(0, 3, R"({"hours": 1, "hours_relative": true, "minutes": 30, "minutes_relative": true})")}},
            {"через полминуты", {CE(0, 2, R"({"seconds": 30, "seconds_relative": true})")}},
            {"через полторы минуты",
             {CE(0, 3, R"({"minutes": 1, "minutes_relative": true, "seconds": 30, "seconds_relative": true})")}},
            {"полчаса назад",
             {CE(0, 2, R"({"minutes": -30, "minutes_relative": true})")}},
            {"полтора часа назад",
             {CE(0, 3, R"({"hours": -1, "hours_relative": true, "minutes": -30, "minutes_relative": true})")}},
            {"полминуты назад",
             {CE(0, 2, R"({"seconds": -30, "seconds_relative": true})")}},
            {"полторы минуты назад",
             {CE(0, 3, R"({"minutes": -1, "minutes_relative": true, "seconds": -30, "seconds_relative": true})")}},
            {"через пару часов", {CE(0, 3, R"({"hours": 2, "hours_relative": true})")}},
            {"пару минут назад", {CE(0, 3, R"({"minutes": -2, "minutes_relative": true})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(Now) {
        const TTestCase testCases[] = {
            {"яндекс сейчас", {CE(1, 2, R"({"seconds": 0, "seconds_relative": true})")}},
            {"погода в настоящее время", {CE(1, 4, R"({"seconds": 0, "seconds_relative": true})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeMandatoryContext) {
        const TTestCase testCases[] = {
            {"завтра на 8 утра", {CE(0, 4, R"({"days": 1, "days_relative": true, "hours": 8, "minutes": 0})")}},
            {"завтра на 8", {CE(0, 3, R"({"days": 1, "days_relative": true, "hours": 8, "minutes": 0})")}},
            {"на 8", {}},
            {"на 8 с половиной утра", {CE(1, 3, R"({"hours": 8, "minutes": 30})")}},
            {"на час дня", {CE(1, 3, R"({"hours": 13, "minutes": 0})")}},
            {"на два дня", {}},
            {"на два часа дня", {CE(1, 4, R"({"hours": 14, "minutes": 0})")}},
            {"суббота 5 число или нет", {CE(0, 1, R"({"weekday": 6})"), CE(1, 3, R"({"days": 5})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeHoursAndHalf) {
        const TTestCase testCases[] = {
            {"8 с половиной вечера", {CE(0, 2, R"({"hours": 20, "minutes": 30})")}},
            {"завтра в 8 с половиной вечера",
             {CE(0, 4, R"({"hours": 20, "minutes": 30, "days": 1, "days_relative": true})")}},
            {"8 с половиной часов вечера", {CE(0, 3, R"({"hours": 20, "minutes": 30})")}},
            {"завтра в 8 с половиной часов вечера",
             {CE(0, 5, R"({"hours": 20, "minutes": 30, "days": 1, "days_relative": true})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(TimeSpecDay) {
        const TTestCase testCases[] = {
            {"на завтра", {CE(1, 2, R"({"days": 1, "days_relative": true})")}},
            {"погода на завтрашний день", {CE(2, 4, R"({"days": 1, "days_relative": true})")}},
            {"послезавтрашняя погода", {CE(0, 1, R"({"days": 2, "days_relative": true})")}},
            {"завтрашняя погода", {CE(0, 1, R"({"days": 1, "days_relative": true})")}},
            {"вчерашняя погода", {CE(0, 1, R"({"days": -1, "days_relative": true})")}},
            {"позавчерашняя погода", {CE(0, 1, R"({"days": -2, "days_relative": true})")}},
            {"погода к завтрашнему дню", {CE(2, 4, R"({"days": 1, "days_relative": true})")}},
            {"что вчера было", {CE(1, 2, R"({"days": -1, "days_relative": true})")}},
            {"курс за вчерашний день", {CE(2, 4, R"({"days": -1, "days_relative": true})")}},
            {"вчера вечером в пять", {CE(0, 4, R"({"days": -1, "days_relative": true, "hours": 17, "minutes": 0})")}},
            {"утро вчерашнего дня", {CE(1, 3, R"({"days": -1, "days_relative": true})")}}
        };
        T().Run(testCases);
    }

} // test suite
