#include "common.h"

#include <alice/nlu/libs/fst/archive_data_loader.h>
#include <alice/nlu/libs/fst/fst_normalizer.h>
#include <alice/nlu/libs/fst/fst_num.h>
#include <alice/nlu/libs/fst/prefix_data_loader.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>


using namespace NAlice;
using namespace NAlice::NTestHelpers;


Y_UNIT_TEST_SUITE(TFstNumTrTests) {

    static const TTestCaseRunner<TFstNum>& T() {
        static const TTestCaseRunner<TFstNum> T{"NUM", LANG_TUR};
        return T;
    }

    static TEntity CE(size_t begin, size_t end, int64_t value) {
        return CreateEntity(begin, end, "NUM", value);
    }

    Y_UNIT_TEST(ParseEmpty) {
        const TTestCase testCases[] = {
            {"", {}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(Num) {
        const TTestCase testCases[] = {
            {"hacim eksi iki", {CE(1, 3, -2)}},
            {"hacim -2", {CE(1, 2, -2)}},
            {"bin yüz yirmi üç", {CE(0, 1, 1123)}},
            {"on üç", {CE(0, 1, 13)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(NumCurrency) {
        const TTestCase testCases[] = {
            {"1 bin dolara eşit 6 bin beş yüz lira", {CE(0, 1, 1000), CE(3, 4, 6500)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(NumWithinDateTime) {
        const TTestCase testCases[] = {
            {"5 Aralık 2015'te 5 kişilik oda", {CE(0, 1, 5), CE(2, 3, 2015), CE(4, 5, 5)}}
        };
        T().Run(testCases);
    }

} // test suite
