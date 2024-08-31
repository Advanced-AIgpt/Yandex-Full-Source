#include "common.h"

#include <alice/nlu/libs/fst/fst_time.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NTestHelpers;

Y_UNIT_TEST_SUITE(TFstTimeTests) {

    static const TTestCaseRunner<TFstTime>& T() {
        static const TTestCaseRunner<TFstTime> T{"TIME", LANG_RUS};
        return T;
    }

    static TEntity CE(size_t begin, size_t end, TStringBuf json) {
        return CreateEntity(begin, end, "TIME", NSc::TValue::FromJsonThrow(json));
    }

    Y_UNIT_TEST(ParseEmpty) {
        const TTestCase testCases[] = {
            {"", {}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(ParseWithMarkup) {
        const TTestCase testCases[] = {
            {"встретимся послезавтра, около десяти минут шестого вечера!",
             {CE(4-1, 8-1, R"({"hours": 5, "minutes": 10, "period": "pm"})")}},
            {"ноль минут ноль секунд",
             {CE(1-1, 5-1, R"({"seconds": 0, "minutes": 0})")}},
            {"половина пятого вечера",
             {CE(1-1, 4-1, R"({"hours": 4, "minutes": 30, "period": "pm"})")}},
            {"встретимся послезавтра приблизительно в 17 10",
             {CE(5-1, 7-1, R"({"hours": 17, "minutes": 10})")}},
            {"встретимся послезавтра, в без пятнадцати шесть вечера",
             {CE(4-1, 8-1, R"({"hours": 5, "minutes": 45, "period": "pm"})")}},
            {"встретимся послезавтра в без 15 6 вечера",
             {CE(4-1, 8-1, R"({"hours": 5, "minutes": 45, "period": "pm"})")}},
            {"машину на без 15 6 завтра", {CE(3-1, 6-1, R"({"hours": 5, "minutes": 45})")}},
            {"через 3 часа, 9 минут, 15 секунд приедет поезд",
             {CE(1-1, 8-1, R"({"time_relative": true, "hours": 3, "minutes": 9, "seconds": 15})")}},
            {"08.03.2016, в 10:30 будет праздник!!", {CE(3-1, 4-1, R"({"hours": 10, "minutes": 30})")}},
/*'could be fixed after DIALOG-1324'
  {"первого мая - день труда, в 3 после полудня на картошку .",
              {CE(6, 9, R"({"hours": 3, "period": "pm"})")}},*/
            {"машину на полшестого вечера", {CE(3-1, 5-1, R"({"hours": 5, "minutes": 30, "period": "pm"})")}},
            {"машину на полшестого утра", {CE(3-1, 5-1, R"({"hours": 5, "minutes": 30, "period": "am"})")}},
            {"машину на 6 30 утра", {CE(3-1, 6-1, R"({"hours": 6, "minutes": 30, "period": "am"})")}},
            {"машину на 6:30 утра", {CE(3-1, 5-1, R"({"hours": 6, "minutes": 30, "period": "am"})")}},
            {"машину к вечеру на шесть сорок пять ", {CE(3-1, 7-1, R"({"hours": 6, "minutes": 45, "period": "pm"})")}},
            {"машину на 7 часов 30 минут", {CE(3-1, 7-1, R"({"hours": 7, "minutes": 30})")}},
            {"будильник на 6 утра завтра", {CE(3-1, 5-1, R"({"hours": 6, "period": "am"})")}},
            {"были там позавчера вечером примерно около девяти тридцати",
             {CE(4-1, 9-1, R"({"hours": 9, "minutes": 30, "period": "pm"})")}},
            {"машину в полдень", {CE(3-1, 4-1, R"({"hours": 12, "minutes": 0, "period": "pm"})")}},
            {"мне нужно такси в полночь", {CE(5-1, 6-1, R"({"hours": 12, "minutes": 0, "period": "am"})")}},
            {"вчера в 10 минут первого утра", {CE(3-1, 7-1, R"({"hours": 12, "minutes": 10, "period": "pm"})")}},
            {"в следующую субботу в без пятнадцати шесть вечера",
             {CE(5-1, 9-1, R"({"hours": 5, "minutes": 45, "period": "pm"})")}},
            {"четверть восьмого утра в прошлую среду", {CE(1-1, 4-1, R"({"hours": 7, "minutes": 15, "period": "am"})")}},
            {"четверть восьмого утра в среду", {CE(1-1, 4-1, R"({"hours": 7, "minutes": 15, "period": "am"})")}},
            {"четверть восьмого утра в эту среду", {CE(1-1, 4-1, R"({"hours": 7, "minutes": 15, "period": "am"})")}},
            {"в ближайшую среду на четверть восьмого утра",
             {CE(5-1, 8-1, R"({"hours": 7, "minutes": 15, "period": "am"})")}},
            {"давай лучше в 20:00", {CE(4-1, 5-1, R"({"hours": 20, "minutes": 0})")}},
            {"в двадцать ноль ноль", {CE(1, 4, R"({"hours": 20, "minutes": 0})")}},
            {"в двадцать один тридцать два", {CE(1, 3, R"({"hours": 21, "minutes": 32})")}},
            {"в первом часу дня", {CE(2-1, 5-1, R"({"hours": 1, "period": "pm"})")}},
            {"во втором часу дня", {CE(2-1, 5-1, R"({"hours": 2, "period": "pm"})")}},
            {"Поставь таймер на 5 минут 30 секунд", {CE(4-1, 8-1, R"({"minutes": 5, "seconds": 30})")}},
            {"Напомни через 2 с половиной минуты",
             {CE(2-1, 5-1, R"({"minutes": 2, "minutes_relative": true, "seconds": 30, "seconds_relative": true})")}},
            {"Установи время на 13 часов 20 минут 42 секунды",
             {CE(4-1, 10-1, R"({"hours": 13, "minutes": 20, "seconds": 42})")}},
            {"Через 2 минуты и 30 секунд напомни",
             {CE(1-1, 7-1, R"({"minutes": 2, "minutes_relative": true, "seconds": 30, "seconds_relative": true})")}},
            {"поставь будильник на половину восьмого", {CE(4-1, 6-1, R"({"hours": 7, "minutes": 30})")}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(WithoutMarkup) {
        const auto v =
            [] (const TStringBuf json) {
                return NSc::TValue::FromJsonThrow(json);
            };
        const TTestCaseValue testCases[] = {
            {"будильник на 6", {v(R"({"hours": 6})")}},
            {"будильник на 6 45", {v(R"({"hours": 6, "minutes": 45})")}},
            {"будильник на 19 03", {v(R"({"hours": 19, "minutes": 3})")}},
            {"будильник на 19 0 3", {v(R"({"hours": 19, "minutes": 3})")}},
            {"будильник через 3 часа", {v(R"({"hours": 3, "hours_relative": true})")}},
            {"будильник на 15 минут", {v(R"({"minutes": 15})")}},
            {"будильник на 3 часа 15 минут", {v(R"({"hours": 3, "minutes": 15})")}},
            {"будильник на 3 часа 05 минут", {v(R"({"hours": 3, "minutes": 5})")}},
            {"будильник на 3 часа 0 5 минут", {v(R"({"hours": 3, "minutes": 5})")}},
            {"будильник на 8 часов 15 минут вечера", {v(R"({"hours": 8, "minutes": 15, "period": "pm"})")}},
            {"будильник на час 30", {v(R"({"hours": 1, "minutes": 30})")}},
            {"будильник на час 30 утра", {v(R"({"hours": 1, "minutes": 30, "period": "am"})")}},
            {"будильник на час 0 2 утра", {v(R"({"hours": 1, "minutes": 2, "period": "am"})")}},
            // test timer
            {"таймер на 42 секунды", {v(R"({"seconds": 42})")}},
            {"таймер на 3 минуты", {v(R"({"minutes": 3})")}},
            {"таймер через 3 минуты", {v(R"({"minutes": 3, "minutes_relative": true})")}},
            {"таймер на 10 секунд", {v(R"({"seconds": 10})")}},
            {"таймер через 10 секунд", {v(R"({"seconds": 10, "seconds_relative": true})")}},
            // special hours mins
            {"через полчаса", {v(R"({"minutes": 30, "minutes_relative": true})")}},
            {"через полтора часа", {v(R"({"hours": 1, "hours_relative": true, "minutes": 30, "minutes_relative": true})")}},
            {"через полминуты", {v(R"({"seconds": 30, "seconds_relative": true})")}},
            {"через полторы минуты", {v(R"({"minutes": 1, "minutes_relative": true, "seconds": 30, "seconds_relative": true})")}},
            {"полчаса назад", {v(R"({"minutes": -30, "minutes_relative": true})")}},
            {"полтора часа назад", {v(R"({"hours": -1, "hours_relative": true, "minutes": -30, "minutes_relative": true})")}},
            {"полминуты назад", {v(R"({"seconds": -30, "seconds_relative": true})")}},
            {"полторы минуты назад", {v(R"({"minutes": -1, "minutes_relative": true, "seconds": -30, "seconds_relative": true})")}},
            {"через пару часов", {v(R"({"hours": 2, "hours_relative": true})")}},
            {"пару минут назад", {v(R"({"minutes": -2, "minutes_relative": true})")}},
            // now
//! doesn't work in python            {"яндекс сейчас", {v(R"({"seconds": 0, "seconds_relative": true})")}},
//! doesn't work in python            {"погода в настоящее время", {v(R"({"seconds": 0, "seconds_relative": true})")}},
            // mandatory context
            {"завтра на 8 утра", {v(R"({"hours": 8, "period": "am"})")}},
            {"завтра на 8", {v(R"({"hours": 8})")}},
            {"на 8", {v(R"({"hours": 8})")}},
//! doesn't work in python            {"на 8 с половиной утра", {v(R"({"hours": 8, "minutes": 30, "period": "am"})")}},
            {"в 11 дня", {v(R"({"hours": 11, "period": "am"})")}},
            {"в 12 дня", {v(R"({"hours": 12, "period": "pm"})")}},
            {"на час дня", {v(R"({"hours": 1, "period": "pm"})")}},
            {"на два дня", {v(R"({"hours": 2, "period": "pm"})")}},
            {"на два часа дня", {v(R"({"hours": 2, "period": "pm"})")}},
//! doesn't work in python            {"8 с половиной вечера", {v(R"({"hours": 8, "minutes": 30, "period": "pm"})")}},
//! doesn't work in python            {"завтра в 8 с половиной вечера", {v(R"({"hours": 8, "minutes": 30, "period": "pm"})")}},
            {"8 с половиной часов вечера", {v(R"({"hours": 8, "minutes": 30, "period": "pm"})")}},
            {"завтра в 8 с половиной часов вечера", {v(R"({"hours": 8, "minutes": 30, "period": "pm"})")}},
            {"вчера вечером в пять", {v(R"({"hours": 5, "period": "pm"})")}},
            {"половина первого", {v(R"({"hours": 0, "minutes": 30})")}},
            {"половина первого ночи", {v(R"({"hours": 12, "minutes": 30, "period": "am"})")}},
            {"половина двенадцатого ночи", {v(R"({"hours": 11, "minutes": 30, "period": "pm"})")}},
            {"четверть первого", {v(R"({"hours": 0, "minutes": 15})")}},
            {"четверть второго", {v(R"({"hours": 1, "minutes": 15})")}},
            {"первый час ночи", {v(R"({"hours": 1, "period": "am"})")}},
            {"в первом часу дня", {v(R"({"hours": 1, "period": "pm"})")}},
            {"во втором часу дня", {v(R"({"hours": 2, "period": "pm"})")}},
            {"будильник на шесть pm", {v(R"({"hours": 6, "period": "pm"})")}},
            {"программа ю беременна в 16 по телевизору 15 минут 0 8 показывают",
             {v(R"({"hours": 16})"), v(R"({"minutes": 15, "seconds": 8})")}},
            {"4 минуты 0 4", {v(R"({"minutes": 4, "seconds": 4})")}},
            {"7 минут 0 5 минут", {v(R"({"minutes": 7, "seconds": 0})"), v(R"({"minutes": 5})")}},
            {"оля пробежала дистанцию за 11 минут 0 5 секунд а таня опередила олю на 10 секунд за какое время пробежала "
             "дистанцию таня реши задачу", {v(R"({"minutes": 11, "seconds": 5})"), v(R"({"seconds": 10})")}},
            {"поставь будильник на половину восьмого", {v(R"({"hours": 7, "minutes": 30})")}},
            {"поставь будильник на полпервого", {v(R"({"hours": 0, "minutes": 30})")}},
            {"поставь будильник на полпервого утра", {v(R"({"hours": 12, "minutes": 30, "period": "pm"})")}},
            {"поставь будильник на полпервого дня", {v(R"({"hours": 12, "minutes": 30, "period": "pm"})")}},
            {"поставь будильник на полпервого ночи", {v(R"({"hours": 12, "minutes": 30, "period": "am"})")}},
            {"с четверти третьего ждем гостей", {v(R"({"hours": 2, "minutes": 15})")}},
            // minuts_to
            {"без пятнадцати 8", {v(R"({"hours": 7, "minutes": 45})")}},
            {"без пяти три", {v(R"({"hours": 2, "minutes": 55})")}},
            {"без четверти 1", {v(R"({"hours": 0, "minutes": 45})")}},
            {"без четверти час", {v(R"({"hours": 0, "minutes": 45})")}},
            {"без пятнадцати час", {v(R"({"hours": 0, "minutes": 45})")}},
            {"без 8 минут час", {v(R"({"hours": 0, "minutes": 52})")}},
            // time of day in the middle
            {"восемь утра пять минут", {v(R"({'hours': 8, 'minutes': 5, 'period': 'am'})")}},
            {"шесть вечера тридцать", {v(R"({'hours': 6, 'minutes': 30, 'period': 'pm'})")}},
            {"пять утра пятнадцать минут тридцать секунд", {v(R"({'hours': 5, 'minutes': 15, 'seconds': 30, 'period': 'am'})")}},
            {"час дня двадцать", {v(R"({'hours': 1, 'minutes': 20, 'period': 'pm'})")}},
            {"три ночи три минуты", {v(R"({'hours': 3, 'minutes': 3, 'period': 'am'})")}},
            {"6 утра 6 15", {
                v(R"({'hours': 6, 'period': 'am'})"),
                v(R"({'hours': 6, 'minutes': 15})"),
            }},
        };
        T().Run(testCases);
    }

} // test suite
