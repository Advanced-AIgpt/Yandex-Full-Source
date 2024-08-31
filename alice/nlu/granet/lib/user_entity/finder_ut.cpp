#include "finder.h"
#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <alice/nlu/granet/lib/sample/tag.h>
#include <alice/nlu/libs/ut_utils/ut_utils.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>

using namespace NGranet;
using namespace NGranet::NUserEntity;

Y_UNIT_TEST_SUITE(NUserEntity_TEntityFinder) {

    TEntityDicts MakeTestDicts() {
        TEntityDicts dicts;
        dicts.AddDict("type1").Items = {
            {"value1", "", "Привет мир!"},
            {"value2", "", "давай-ка"},
            {"value2", "", "Ещё ещё ещё"},
        };
        dicts.AddDict("type2").Items = {
            {R"({"value": "as", "a": "json"})", "", "всё"},
        };
        dicts.AddDict("number").Items = {
            {"23", "", "23"},
            {"23", "", "двадцать три"},
            {"31", "", "тридцать один"},
        };
        dicts.AddDict("movie", EDF_ENABLE_PARTIAL_MATCH).Items = {
            {"1", "visible", "Терминатор"},
            {"2", "visible", "Терминатор 2: Судный день"},
            {"3", "visible", "Начало"},
            {"4", "visible", "1+1"},
            {"5", "visible", "Мальчишник в Вегасе"},
            {"6", "visible", "Мальчишник 2: Из Вегаса в Бангкок"},
            {"7", "", "Оно"},
            {"8", "", "Прочь"},
            {"9", "", "Три билборда на границе Эббинга, Миссури"},
            {"10", "", "Стражи Галактики"},
            {"11", "", "Стражи Галактики. Часть 2"},
        };
        return dicts;
    }

    void TestDictDump(const TEntityDicts& dicts, TStringBuf expected) {
        TStringStream actual;
        dicts.Dump(&actual);
        NAlice::NUtUtils::TestEqual("", expected, actual.Str());
    }

    void TestOnSample(const TEntityDicts& dicts, TStringBuf expected) {
        const TString text = RemoveTaggerMarkup(expected);
        TSample::TRef sample = TSample::Create(text, LANG_RUS);
        sample->AddEntitiesOnTokens(FindEntitiesInSample(dicts, sample, LANG_RUS));
        const TString actual = sample->GetEntitiesAsMarkup("", false).PrintMarkup(SPO_NEED_ALL);
        NAlice::NUtUtils::TestEqual(text, expected, actual);
    }

    Y_UNIT_TEST(MovieResolve) {
        TEntityDicts dicts;
        dicts.AddDict("movie", EDF_ENABLE_PARTIAL_MATCH).Items = {
            {"1", "", "Терминатор"},
            {"2", "", "Терминатор 2: Судный день"},
        };
        TestOnSample(dicts, "Алиса поставь 'Терминатор два'(movie:2)");
    }

    Y_UNIT_TEST(SaveLoad) {
        const TEntityDicts dicts = MakeTestDicts();

        const TStringBuf expected = R"(
            TEntityDicts:
              Dicts:
                TEntityDict:
                  EntityName: type1
                  Flags: 0
                  Items (value extra: text):
                    value1 : Привет мир!
                    value2 : давай-ка
                    value2 : Ещё ещё ещё
                TEntityDict:
                  EntityName: type2
                  Flags: 0
                  Items (value extra: text):
                    {"value": "as", "a": "json"} : всё
                TEntityDict:
                  EntityName: number
                  Flags: 0
                  Items (value extra: text):
                    23 : 23
                    23 : двадцать три
                    31 : тридцать один
                TEntityDict:
                  EntityName: movie
                  Flags: EDF_ENABLE_PARTIAL_MATCH
                  Items (value extra: text):
                    1 visible: Терминатор
                    2 visible: Терминатор 2: Судный день
                    3 visible: Начало
                    4 visible: 1+1
                    5 visible: Мальчишник в Вегасе
                    6 visible: Мальчишник 2: Из Вегаса в Бангкок
                    7 : Оно
                    8 : Прочь
                    9 : Три билборда на границе Эббинга, Миссури
                    10 : Стражи Галактики
                    11 : Стражи Галактики. Часть 2
        )";

        TestDictDump(dicts, expected);

        const TString base64 = dicts.ToBase64();
        TEntityDicts loaded;
        loaded.FromBase64(base64);
        TestDictDump(loaded, expected);
    }

    Y_UNIT_TEST(FinderBasic) {
        const TEntityDicts dicts = MakeTestDicts();

        const TVector<TString> request = {
            "Привет миру давай ка двадцать третий и давай ка тридцать одному eще еще еще еще еще еще Всё",
            "привет миру давай ка 23 и давай ка 31 eще еще еще еще еще еще все"
        };

        TVector<TEntity> expected = {
            {{0, 2},   "type1",  "value1", "", NEntitySources::USER_ENTITY_FINDER, -2.},
            {{2, 4},   "type1",  "value2", "", NEntitySources::USER_ENTITY_FINDER, -2.},
            {{4, 6},   "number", "23",     "", NEntitySources::USER_ENTITY_FINDER, -2.},
            {{7, 9},   "type1",  "value2", "", NEntitySources::USER_ENTITY_FINDER, -2.},
            {{9, 11},  "number", "31",     "", NEntitySources::USER_ENTITY_FINDER, -2.},
            {{11, 14}, "type1",  "value2", "", NEntitySources::USER_ENTITY_FINDER, -2.},
            {{14, 17}, "type1",  "value2", "", NEntitySources::USER_ENTITY_FINDER, -2.},
            {{17, 18}, "type2", R"({"value": "as", "a": "json"})", "", NEntitySources::USER_ENTITY_FINDER, -2.},
        };
        ::Sort(expected);

        TVector<TEntity> actual1 = FindEntitiesInTexts(dicts, request, request[0], GRANET_LANGUAGES);
        ::Sort(actual1);
        UNIT_ASSERT_EQUAL(JoinSeq("\n", expected), JoinSeq("\n", actual1));

        const TString base64 = dicts.ToBase64();
        TEntityDicts clone;
        clone.FromBase64(base64);

        TVector<TEntity> actual2 = FindEntitiesInTexts(clone, request, request[0], GRANET_LANGUAGES);
        ::Sort(actual2);
        UNIT_ASSERT_EQUAL(expected, actual2);
    };

    Y_UNIT_TEST(FindMovies) {
        const TEntityDicts dicts = MakeTestDicts();

        const TStringBuf markups[] = {
            "Алиса поставь 'Терминатора'(movie:1)",
            "Алиса поставь 'Терминатор два'(movie:2)",
            "Включи 'второго терминатора'(movie:2) пожалуйста",
            "Включи 'Терминатор два судный день'(movie:2) пожалуйста",
            "'Начало'(movie:3)",
            "Сначала",
            "К 'началу'(movie:3)",
            "К его 'началу'(movie:3)", // not good
            "Включи фильм 'один плюс один'(movie:4)",
            "Включи 'мальчишник в вегасе'(movie:5)",
            "Включи 'мальчишник в вегасе два'(movie:6)",
            "Включи 'второй мальчишник в вегасе'(movie:6)",
            "Включи 'мальчишник два'(movie:6)",
            "Включи фильм 'оно'(movie:7)",
            "Включи ему",
            "'Три билборда на границе Эббинга'(movie:9)",
            "'Три билборда на границе Эббинга Миссури'(movie:9)",
            "Включи 'стражи галактики часть два'(movie:11)",
            "Включи 'вторую часть стражей галактики'(movie:11)",
            "Включи 'стражи галактики'(movie:10) два", // error
        };

        for (const TStringBuf& markup : markups) {
            TestOnSample(dicts, markup);
        }
    };
}
