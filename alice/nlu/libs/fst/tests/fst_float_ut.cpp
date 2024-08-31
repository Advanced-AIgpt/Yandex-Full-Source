#include "common.h"

#include <alice/nlu/libs/fst/archive_data_loader.h>
#include <alice/nlu/libs/fst/fst_float.h>
#include <alice/nlu/libs/fst/fst_normalizer.h>
#include <alice/nlu/libs/fst/prefix_data_loader.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/memory/blob.h>
#include <util/string/cast.h>

extern "C" {
    extern const ui8 FloatFstData[];
    extern const ui32 FloatFstDataSize;

}

using namespace NAlice;
using namespace NAlice::NTestHelpers;

Y_UNIT_TEST_SUITE(TFstFloatTests) {

    static const TTestCaseRunner<TFstFloat>& T() {
        static const TTestCaseRunner<TFstFloat> T{"FLOAT", LANG_RUS};
        return T;
    }

    static TEntity CE(size_t begin, size_t end, double value) {
        return CreateEntity(begin, end, "FLOAT", value);
    }

    Y_UNIT_TEST(ParseEmpty) {
        const TTestCase testCases[] = {
            {"", {}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(ParseBasic) {
        const TTestCase testCases[] = {
            {"пи равно три целых четырнадцать сотых", {CE(2, 3, 3.14)}},
            {"пи равно 3 целых четырнадцать сотых", {CE(2, 3, 3.14)}},
            {"пи равно три целых 14 сотых", {CE(2, 3, 3.14)}},
            {"пи равно 3.14", {CE(2, 3, 3.14)}},
            {"одна миллионная доллара в рублях", {CE(0, 1, 0.000001)}},
            {"поставь волну сто и пять", {CE(2,5, 100.5)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(ParseFractionalWithDivisionByZero) {
        const auto& normalizer = CreateNormalizer(LANG_RUS);
        struct TSpecialTestCase : public TTestCase
        {
            bool UseNormalizer;
        };

        const TSpecialTestCase testCases[] = {
            {{"49/0 5", {}}, true},
            {{"49/0 5", {}}, false},
            {{"49 / 0 5", {}}, true},
            {{"заинск проспект победы 1/0 6 а", {}}, true},
            {{"заинск проспект победы 1/0 6 а", {}}, false},
            {{"разбуди меня в шесть ноль ноль", {}}, true},
            {{"напомни мне завтра в 3 00 взять с собой лестницу", {}}, true}
        };
        auto reader = MakeHolder<TArchiveReader>(TBlob::NoCopy(FloatFstData, FloatFstDataSize));
        TArchiveDataLoader loader{std::move(reader)};
        TFstFloat fst{loader};
        for (const auto& testCase : testCases) {
            const auto normalized = testCase.UseNormalizer
                ? normalizer.Normalize(ToString(testCase.Name))
                : ToString(testCase.Name);
            auto&& entities = fst.Parse(normalized);
            PrintEntities(entities);
            DropExcept("FLOAT", &entities);
            PrintEntities(entities);
            UNIT_ASSERT(Equal(entities, testCase.Entities));
        }
    }

    Y_UNIT_TEST(ParseBasicFail) {
        const TTestCase testCases[] = {
            {"1 миллионная доллара в рублях", {}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(FloatInflected) {
        const TTestCase testCases[] = {
            {"пи равно трем целым четырнадцати сотым", {CreateEntity(2, 3, "FLOAT", 3.14)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(Fraction) {
        const TTestCase testCases[] = {
            {"пять сотых рублей", {CreateEntity(0, 1, "FLOAT", 0.05)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(FractionInflected) {
        const TTestCase testCases[] = {
            {"пятью сотыми рублями", {CreateEntity(0, 1, "FLOAT", 0.05)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(NumberAndQuarter) {
        const TTestCase testCases[] = {
            {"пять с четвертью рублей", {CreateEntity(0, 1, "FLOAT", 5.25)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(NumberAndHalf) {
        const TTestCase testCases[] = {
            {"пять с половиной рублей", {CreateEntity(0, 1, "FLOAT", 5.5)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(IntegerFractionalPart) {
        const TTestCase testCases[] = {
            {"с вас три пятьдесят", {CreateEntity(2, 4, "FLOAT", 3.50)}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(ZeroNegative) {
        const TTestCase testCases[] = {
            {"0,0 кельвинов это -273.15 градуса цельсия", {
                    CreateEntity(0, 1, "FLOAT", 0.0),
                    CreateEntity(3, 4, "FLOAT", -273.15)
                }
            }
        };
        T().Run(testCases);
    }


}
