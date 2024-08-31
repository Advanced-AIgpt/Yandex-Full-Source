#include "common.h"

#include <alice/nlu/libs/fst/archive_data_loader.h>
#include <alice/nlu/libs/fst/fst_geo.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/string/cast.h>

extern "C" {
    extern const ui8 GeoFstData[];
    extern const ui32 GeoFstDataSize;

}

using namespace NAlice;
using namespace NAlice::NTestHelpers;

Y_UNIT_TEST_SUITE(TFstGeoTests) {

    Y_UNIT_TEST(ParseEmpty) {
        auto reader = MakeHolder<TArchiveReader>(TBlob::NoCopy(GeoFstData, GeoFstDataSize));
        TArchiveDataLoader loader{std::move(reader)};
        TFstGeo fst{loader};
        auto&& entities = fst.Parse("");
        UNIT_ASSERT(entities.empty());
    }

    Y_UNIT_TEST(ParseTest1) {
        auto reader = MakeHolder<TArchiveReader>(TBlob::NoCopy(GeoFstData, GeoFstDataSize));
        TArchiveDataLoader loader{std::move(reader)};
        TFstGeo fst{loader};
        auto&& entities = fst.Parse("президент россии в в путин");
        DropExcept("GEO", &entities);
        PrintEntities(entities);
        TVector<TEntity> correct = {
            CreateEntity(
                1,
                2,
                "GEO",
                NSc::TValue::FromJsonThrow(R"json({"country":{"id":225,"name":"Россия"}})json"))};
        PrintEntities(correct);
        UNIT_ASSERT(Equal(correct, entities));
    }

#define TC(request, start, end, json) TTestCase{request, {CreateEntity(start, end, "GEO", \
            NSc::TValue::FromJsonThrow(json))}}

    Y_UNIT_TEST(ParseGeos) {
        const auto ce = [] (size_t start, size_t end, TStringBuf json) {
            return CreateEntity(start, end, "GEO", NSc::TValue::FromJsonThrow(json));
        };
        TTestCase testCases[] = {
            TC("по адресу москва ленинградский проспект 6", 2, 6,
                "{\"city\": {\"id\": 213, \"name\": \"Москва\"},\"street\": \"ленинградский\",\"house\": 6}"),
            {"лондон - столица великобритании или англии?", {
                    ce(0, 1, "{\"city\": {\"id\": 103511, \"name\": \"Лондон\"}}"),
                    ce(3, 4, "{\"country\":{\"id\":102,\"name\":\"Великобритания\"}}")}},
            TC("буду в питере", 2, 3, "{\"city\": {\"id\": 2, \"name\": \"Санкт-Петербург\"}}"),
            {"закажи мне такси от улицы льва толстого дом 16 до одинцово через 15 минут", {
             ce(4, 9, "{\"street\": \"льва толстого\",\"house\": 16}"),
             ce(10, 11, "{\"city\": {\"id\": 10743, \"name\": \"Одинцово\"}}")}},
            TC("от улицы дом 16 до", 1, 4, "{\"street\": \"дом\", \"house\": 16}"),
            TC("от улицы льва николаевича толстого дом 16 до", 1, 7,
                "{\"street\": \"льва николаевича толстого\",\"house\": 16}"),
            {"от улицы графа льва николаевича толстого дом 16 до", {}},
            TC("машину от улицы 8 марта дом 3", 2, 7, "{\"street\": \"8 марта\", \"house\": 3}"),
            TC("машину от улицы 26 бакинских комиссаров дом 3", 2, 8,
                "{\"street\": \"26 бакинских комиссаров\",\"house\": 3}")
        };

        auto reader = MakeHolder<TArchiveReader>(TBlob::NoCopy(GeoFstData, GeoFstDataSize));
        TArchiveDataLoader loader{std::move(reader)};
        TFstGeo fst{loader};
        for (const auto& testCase : testCases) {
            auto&& entities = fst.Parse(ToString(testCase.Name));
            DropExcept("GEO", &entities);
            PrintEntities(entities);
            UNIT_ASSERT(Equal(entities, testCase.Entities));
        }
    }
#undef TC

}
