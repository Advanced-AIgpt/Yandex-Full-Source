#include "common.h"

#include <alice/nlu/libs/fst/archive_data_loader.h>
#include <alice/nlu/libs/fst/fst_calc.h>
#include <alice/nlu/libs/fst/fst_normalizer.h>
#include <alice/nlu/libs/fst/prefix_data_loader.h>
#include <alice/nlu/libs/fst/tokenize.h>

using namespace NAlice;
using namespace NAlice::NTestHelpers;

Y_UNIT_TEST_SUITE(TFstCalcTests) {

    static const TTestCaseRunner<TFstCalc>& T() {
        static const TTestCaseRunner<TFstCalc> T{"CALC", LANG_RUS};
        return T;
    }

    static TEntity CE(size_t begin, size_t end, TStringBuf value) {
        return CreateEntity(begin, end, "CALC", value);
    }

    Y_UNIT_TEST(ParseEmpty) {
        const TTestCase testCases[] = {
            {"", {}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(Basic) {
        const TTestCase testCases[] = {
            {"69 умножить на 3", {CE(0, 4, "{69}  умножить  на  {3} ")}},
            {"посчитай пи разделить на 2", {CE(1, 5, "пи  разделить  на  {2} ")}},
            {"2 + 2", {CE(0, 3, "{2}  +  {2} ")}},
            {"2/2", {CE(0, 3, "{2}  /  {2} ")}},
            {"вычислить 0.05 + ctan(3.65) / 5.51 на калькуляторе", {CE(1, 7, "{0.05}  +  ctan  {3.65}  /  {5.51}  ")}},
            {"сколько будет cos(0.29)", {CE(2, 4, "cos  {0.29} ")}},
            {"cos(0.29) это сколько", {CE(0, 2, "cos  {0.29}  ")}},
            {"арккосинус шесть восьмых сколько будет", {CE(0, 2, "арккосинус  {6/8}  ")}},
            {"100 умножить на 2 разделить на 8", {CE(0, 7, "{100}  умножить  на  {2}  разделить  на  {8} ")}},
            {"раздели 1003 на 7", {CE(0, 4, "раздели  {1003}  на  {7} ")}},
            {"к 22 прибавь 16", {CE(0, 4, "к  {22}  прибавь  {16} ")}},
            {"пи пополам делить на тысяча десять плюс синус тринадцать",
             {CE(0, 8, "пи  пополам  делить  на  {1010}  плюс  синус  {13} ")}},
            {"минус пять на пи пополам", {CE(0, 5, "минус  {5}  на  пи  пополам ")}},
            {"косинус шесть восьмых умножить синус сто пять", {CE(0, 5, "косинус  {6/8}  умножить  синус  {105} ")}},
            {"трижды девять", {CE(0, 2, "трижды  9 ")}},
            {"трижды девять прибавить семь четвертых", {CE(0, 4, "трижды  9  прибавить  {7/4} ")}}
        };
        T().Run(testCases);
    }

} // test suite

