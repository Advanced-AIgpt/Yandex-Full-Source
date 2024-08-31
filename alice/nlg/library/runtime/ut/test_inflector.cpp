#include <alice/nlg/library/runtime/inflector.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/hash_set.h>
#include <util/string/split.h>

using namespace NAlice::NNlg;
using namespace NAlice::NNlg::NPrivate;

Y_UNIT_TEST_SUITE(NlgInflector) {
    Y_UNIT_TEST(NormalizeInflectionCases) {
        auto toSet = [](const TStringBuf str) {
            THashSet<TString> result;
            StringSplitter(str).Split(',').Collect(&result);
            return result;
        };

        UNIT_ASSERT_VALUES_EQUAL(toSet("sg,f"), toSet(NormalizeInflectionCases("sg,f")));
        UNIT_ASSERT_VALUES_EQUAL(toSet("S,f"), toSet(NormalizeInflectionCases("NOUN,f")));
    }

    Y_UNIT_TEST(NormalInflectorSmoke) {
        const std::unique_ptr<IInflector> inflector = CreateNormalInflector();
        const TString target = "московский государственный университет";
        UNIT_ASSERT_VALUES_EQUAL("московский государственный университет", inflector->Inflect(target, "sg,nomn"));
        UNIT_ASSERT_VALUES_EQUAL("московского государственного университета", inflector->Inflect(target, "sg,gent"));
        UNIT_ASSERT_VALUES_EQUAL("московскому государственному университету", inflector->Inflect(target, "sg,datv"));
        UNIT_ASSERT_VALUES_EQUAL("московский государственный университет", inflector->Inflect(target, "sg,accs"));
        UNIT_ASSERT_VALUES_EQUAL("московском государственном университете", inflector->Inflect(target, "sg,loct"));
        UNIT_ASSERT_VALUES_EQUAL("московским государственным университетом", inflector->Inflect(target, "sg,ablt"));
    }

    Y_UNIT_TEST(NormalInflectorSpaces) {
        const std::unique_ptr<IInflector> inflector = CreateNormalInflector();
        const TString target = " \n \n московский государственный университет \n \n";
        UNIT_ASSERT_VALUES_EQUAL("московский государственный университет", inflector->Inflect(target, "sg,nomn"));
        UNIT_ASSERT_VALUES_EQUAL("московского государственного университета", inflector->Inflect(target, "sg,gent"));
        UNIT_ASSERT_VALUES_EQUAL("московскому государственному университету", inflector->Inflect(target, "sg,datv"));
        UNIT_ASSERT_VALUES_EQUAL("московский государственный университет", inflector->Inflect(target, "sg,accs"));
        UNIT_ASSERT_VALUES_EQUAL("московском государственном университете", inflector->Inflect(target, "sg,loct"));
        UNIT_ASSERT_VALUES_EQUAL("московским государственным университетом", inflector->Inflect(target, "sg,ablt"));
    }

    Y_UNIT_TEST(NormalInflectorGeo) {
        const std::unique_ptr<IInflector> inflector = CreateNormalInflector();
        const TString target = "улицу льва толстого";
        UNIT_ASSERT_VALUES_EQUAL("улица льва толстого", inflector->Inflect(target, "sg,nomn"));
        UNIT_ASSERT_VALUES_EQUAL("улицы льва толстого", inflector->Inflect(target, "sg,gent"));
        UNIT_ASSERT_VALUES_EQUAL("улице льва толстого", inflector->Inflect(target, "sg,datv"));
        UNIT_ASSERT_VALUES_EQUAL("улицу льва толстого", inflector->Inflect(target, "sg,accs"));
        UNIT_ASSERT_VALUES_EQUAL("улице льва толстого", inflector->Inflect(target, "sg,loct"));
        UNIT_ASSERT_VALUES_EQUAL("улицей льва толстого", inflector->Inflect(target, "sg,ablt"));
    }

    Y_UNIT_TEST(NormalInflectorGeoToNominal) {
        const std::unique_ptr<IInflector> inflector = CreateNormalInflector();
        UNIT_ASSERT_VALUES_EQUAL("москва", inflector->Inflect("москве", "nomn"));
        UNIT_ASSERT_VALUES_EQUAL("питер", inflector->Inflect("питере", "nomn"));
        UNIT_ASSERT_VALUES_EQUAL("сан-франциско", inflector->Inflect("сан-франциско", "nomn"));
        UNIT_ASSERT_VALUES_EQUAL("нальчик", inflector->Inflect("нальчике", "nomn"));
        UNIT_ASSERT_VALUES_EQUAL("набережные челны", inflector->Inflect("набережных челнах", "nomn"));
        UNIT_ASSERT_VALUES_EQUAL("магадан", inflector->Inflect("магадане", "nomn"));
        UNIT_ASSERT_VALUES_EQUAL("лондон", inflector->Inflect("лондоне", "nomn"));
    }

    Y_UNIT_TEST(NormalInflectorPlur) {
        const std::unique_ptr<IInflector> inflector = CreateNormalInflector();
        UNIT_ASSERT_VALUES_EQUAL("парогенераторами", inflector->Inflect("парогенератор", "plur,ablt"));
    }

    Y_UNIT_TEST(FioInflector) {
        const std::unique_ptr<IInflector> inflector = CreateFioInflector();
        const TString target = "Миша Парахин";
        UNIT_ASSERT_VALUES_EQUAL("Миша Парахин", inflector->Inflect(target, "m,nomn"));
        UNIT_ASSERT_VALUES_EQUAL("Миши Парахина", inflector->Inflect(target, "m,gent"));
        UNIT_ASSERT_VALUES_EQUAL("Мише Парахину", inflector->Inflect(target, "m,datv"));
        UNIT_ASSERT_VALUES_EQUAL("Мишу Парахина", inflector->Inflect(target, "m,accs"));
        UNIT_ASSERT_VALUES_EQUAL("Мише Парахине", inflector->Inflect(target, "m,loct"));
        UNIT_ASSERT_VALUES_EQUAL("Мишей Парахиным", inflector->Inflect(target, "m,ablt"));
    }

    Y_UNIT_TEST(Pluralize) {
        const std::tuple<TString, std::variant<double, ui64>, TString, TString> cases[] = {
            {"маршрут", static_cast<ui64>(2), "nomn", "маршрута"},
            {"маршрут", static_cast<ui64>(5), "nomn", "маршрутов"},
            {"маршрут", static_cast<ui64>(1), "nomn", "маршрут"},
            {"маршрут", static_cast<ui64>(1), "datv", "маршруту"},
            {"маршрут", static_cast<ui64>(2), "datv", "маршрутам"},
            {"маршрут", static_cast<ui64>(2000), "datv", "маршрутов"},
            {"маршрут", static_cast<double>(2), "datv", "маршрутам"},
            {"маршрут", static_cast<double>(2.1), "datv", "маршрута"},
            {"маршрут", static_cast<ui64>(2), "ins", "маршрутами"},
            {"маршрут", static_cast<ui64>(5), "acc", "маршрутов"},
            {"минута", static_cast<ui64>(1), "nomn", "минута"},
            {"минута", static_cast<ui64>(1), "acc", "минуту"},
            {"минута", static_cast<ui64>(0), "acc", "минут"},
            {"минута", static_cast<ui64>(11), "acc", "минут"},
            {"минута", static_cast<ui64>(21), "acc", "минуту"},
            {"минута", static_cast<ui64>(431), "acc", "минуту"},
        };

        const std::unique_ptr<IInflector> inflector = CreateNormalInflector();
        for (const auto& [target, number, inflCase, expected] : cases) {
            TStringBuilder outNumber;
            struct {
                void operator()(const double value) const {
                    Out << "double(" << value << ")";
                }

                void operator()(const ui64 value) const {
                    Out << "ui64(" << value << ")";
                }

                TStringBuilder& Out;
            } visitor{outNumber};
            std::visit(visitor, number);

            UNIT_ASSERT_VALUES_EQUAL_C(expected,
                                       PluralizeWords(*inflector, target, number, inflCase),
                                       "target = " << target << ", number = " << TString{outNumber});
        }
    }

    Y_UNIT_TEST(Singularize) {
        const std::tuple<TString, ui64, TString> cases[] = {
            {"маршрута", 2, "маршрут"},
            {"маршрутов", 5, "маршрут"},
            {"маршрут", 1, "маршрут"},
        };

        for (const auto& [target, number, expected] : cases) {
            UNIT_ASSERT_VALUES_EQUAL_C(expected, SingularizeWords(target, number), "target = " << target);
        }
    }
}
