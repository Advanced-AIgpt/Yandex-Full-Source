#include "common.h"

#include <alice/nlu/libs/fst/archive_data_loader.h>
#include <alice/nlu/libs/fst/fst_normalizer.h>
#include <alice/nlu/libs/fst/fst_num.h>
#include <alice/nlu/libs/fst/prefix_data_loader.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>


using namespace NAlice;
using namespace NAlice::NTestHelpers;

Y_UNIT_TEST_SUITE(TFstNumRuTests) {

    static const TTestCaseRunner<TFstNum>& T() {
        static const TTestCaseRunner<TFstNum> T{"NUM", LANG_RUS};
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

    Y_UNIT_TEST(NumCurrency) {
        const TTestCase testCases[] = {
            {"67 тысяч рублей равны одной тысячи долларов", {CE(0, 1, 67000), CE(3, 4, 1000)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(NumWithinDateTime) {
        const TTestCase testCases[] = {
            {"номер на 5 человек на 5 декабря 2015", {CE(2, 3, 5), CE(5, 6, 5), CE(7, 8, 2015)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(NumMultiToken) {
        const TTestCase testCases[] = {
            {"хаус 5 сезон 10 серия", {CE(1, 2, 5), CE(3, 4, 10)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(AbnormalPunctuation) {
        const TTestCase testCases[] = {
            {"скачать windows 81 enterprise (x86|x64) matros edition v062015 [ 2015 rus]",
             {CE(2, 3, 81), CE(5, 6, 86), CE(7, 8, 64), CE(11, 12, 62015), CE(12, 13, 2015)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(ZeroNegativeCardinal) {
        const TTestCase testCases[] = {
            {"ноль кельвинов это -273 градуса цельсия",
             {CE(0, 1, 0), CE(3, 4, -273)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(Num) {
        const TTestCase testCases[] = {
            {"громкость минус 2", {CE(1, 3, -2)}},
            {"громкость -2", {CE(1, 2, -2)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(StrangeNoun) {
        const TTestCase testCases[] = {
            {"громкость единичка", {CE(1, 2, 1)}},
            {"громкость на троечку", {CE(2, 3, 3)}},
            {"получил пятак, а не тройбас", {CE(1, 2, 5), CE(4, 5, 3)}}
        };
        T().Run(testCases);
    }

} // test suite

/*
    def test_with_abnormal_punkt(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'скачать windows 81 enterprise (x86|x64) matros edition v062015 [2015 rus]')
        assert r == [
            Entity(start=3, end=4, type='NUM', value=81),
            Entity(start=10, end=11, type='NUM', value=2015)
        ]

    def _assert_inflected(self, parser, samples_extractor, text, expected):
        infl = Inflector('ru')
        for gender in GRAM_GENDER:
            for case in GRAM_CASE:
                assert self(parser, samples_extractor, infl.inflect(text, {gender, case}))[0].value == expected

    @pytest.mark.parametrize("number,expected_value", [
        ('первый', 1), ('второй', 2), ('третий', 3)
    ])
    def test_ordinal(self, parser, samples_extractor, number, expected_value):
        self._assert_inflected(parser, samples_extractor, number, expected_value)

    @pytest.mark.parametrize("nouns,expected_value", [
        ('единица', 1), ('двойка', 2), ('тройка', 3),
        ('четверка', 4), ('пятерка', 5), ('шестерка', 6),
        ('семерка', 7), ('восьмерка', 8), ('девятка', 9),
        ('десятка', 10)
    ])
    def test_nouns(self, parser, samples_extractor, nouns, expected_value):
        self._assert_inflected(parser, samples_extractor, nouns, expected_value)

    def test_zero_negative_cardinal(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'ноль кельвинов это -273 градуса цельсия')
        assert r[0].value == 0
        assert r[1].value == -273

    @pytest.mark.parametrize('input, expected_value', [
        ('громкость минус 2', -2),
        ('громкость -2', -2)
    ])
    def test_num(self, parser, samples_extractor, input, expected_value):
        assert self(parser, samples_extractor, input)[0].value == expected_value
*/
