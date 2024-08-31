#include "entity_parsing.h"

#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/stream/str.h>

namespace NAlice::NNlu {

IOutputStream& operator<<(IOutputStream& out, const TEntityConfig& entity) {
    return out << entity.Name;
}

bool operator==(const TEntityConfig& x, const TEntityConfig& y) {
    return std::tie(x.Name, x.Inflect, x.InflectNumbers, x.Values) ==
           std::tie(y.Name, y.Inflect, y.InflectNumbers, y.Values);
}

bool operator==(const TEntityConfig::TValue& x, const TEntityConfig::TValue& y) {
    return std::tie(x.Value, x.Samples) == std::tie(y.Value, y.Samples);
}

} // namespace NAlice::NNlu

using namespace NAlice::NNlu;

namespace {

const TVector<TEntityConfig> ENTITY_CONFIGS = {
    TEntityConfig{
            .Name = "inflectNumbersFalse",
            .Inflect = true,
            .InflectNumbers = false,
            .Values = {{.Value = "value2", .Samples = {"моя работа"}},
                    {.Value = "value1", .Samples = {"мой дом"}}}
        },
    TEntityConfig{
            .Name = "inflectNumbersTrue",
            .Inflect = true,
            .InflectNumbers = true,
            .Values = {{.Value = "isSingleFalse", .Samples = {"тарифы"}},
                    {.Value = "isSingleTrue", .Samples = {"банк"}}}
        }
    };
} // namespace

Y_UNIT_TEST_SUITE(PrintCustomEntityStringsTest) {
    Y_UNIT_TEST(ReadEntitiesFromJsonTest) {
        const NJson::TJsonValue JSON_ENTITY_CONFIGS = NAlice::JsonFromString(R"(
        [
            {
                "entity": "inflectNumbersFalse",
                "values": {
                    "value2": [
                        "моя работа"
                    ],
                    "value1": [
                        "мой дом"
                    ]
                }
            },
            {
                "entity": "inflectNumbersTrue",
                "inflect_numbers": true,
                "values": {
                    "isSingleTrue": [
                        "банк"
                    ],
                    "isSingleFalse": [
                        "тарифы"
                    ]
                }
            }
        ]
        )");
        TVector<TEntityConfig> result;
        ReadEntitiesFromJson(JSON_ENTITY_CONFIGS, &result);
        UNIT_ASSERT_VALUES_EQUAL(result, ENTITY_CONFIGS);
    }

    Y_UNIT_TEST(MakeEntityStringsTest) {
        const THashSet<TString> ENTITY_TYPES = {"inflectNumbersTrue", "inflectNumbersFalse"};
        TVector<TEntityString> result = MakeEntityStrings(ENTITY_TYPES, ENTITY_CONFIGS);
        Sort(result);
        const TVector<TEntityString> ENTITY_STRINGS = {
            {"банк", "inflectNumbersTrue", "isSingleTrue"},      {"банк", "inflectNumbersTrue", "isSingleTrue"},
            {"банк", "inflectNumbersTrue", "isSingleTrue"},      {"банка", "inflectNumbersTrue", "isSingleTrue"},
            {"банкам", "inflectNumbersTrue", "isSingleTrue"},    {"банками", "inflectNumbersTrue", "isSingleTrue"},
            {"банках", "inflectNumbersTrue", "isSingleTrue"},    {"банке", "inflectNumbersTrue", "isSingleTrue"},
            {"банки", "inflectNumbersTrue", "isSingleTrue"},     {"банки", "inflectNumbersTrue", "isSingleTrue"},
            {"банков", "inflectNumbersTrue", "isSingleTrue"},    {"банком", "inflectNumbersTrue", "isSingleTrue"},
            {"банку", "inflectNumbersTrue", "isSingleTrue"},     {"моего дома", "inflectNumbersFalse", "value1"},
            {"моей работе", "inflectNumbersFalse", "value2"},    {"моей работе", "inflectNumbersFalse", "value2"},
            {"моей работой", "inflectNumbersFalse", "value2"},   {"моей работы", "inflectNumbersFalse", "value2"},
            {"моем доме", "inflectNumbersFalse", "value1"},      {"моему дому", "inflectNumbersFalse", "value1"},
            {"моим домом", "inflectNumbersFalse", "value1"},     {"мой дом", "inflectNumbersFalse", "value1"},
            {"мой дом", "inflectNumbersFalse", "value1"},        {"мой дом", "inflectNumbersFalse", "value1"},
            {"мою работу", "inflectNumbersFalse", "value2"},     {"моя работа", "inflectNumbersFalse", "value2"},
            {"моя работа", "inflectNumbersFalse", "value2"},     {"тарифам", "inflectNumbersTrue", "isSingleFalse"},
            {"тарифами", "inflectNumbersTrue", "isSingleFalse"}, {"тарифах", "inflectNumbersTrue", "isSingleFalse"},
            {"тарифов", "inflectNumbersTrue", "isSingleFalse"},  {"тарифы", "inflectNumbersTrue", "isSingleFalse"},
            {"тарифы", "inflectNumbersTrue", "isSingleFalse"},   {"тарифы", "inflectNumbersTrue", "isSingleFalse"}
        };
        UNIT_ASSERT_VALUES_EQUAL(result, ENTITY_STRINGS);
    }
}
