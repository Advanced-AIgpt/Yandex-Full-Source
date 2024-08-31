#include "entity_finder.h"

#include <library/cpp/json/json_value.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>

using namespace NAlice::NEntitySearch::NEntityFinder;

namespace {

Y_UNIT_TEST_SUITE(EntityFinder) {
    Y_UNIT_TEST(ExtractEntities) {
        auto res = ExtractEntityId("");
        UNIT_ASSERT_VALUES_EQUAL(res.empty(), true);

        TString winner = "минусинск\t0\t1\truw50420\t0.993\tgeo\tfb:location.citytown|fb:location.location\t8";
        TString expected = "ruw50420";
        res = ExtractEntityId(winner);
        UNIT_ASSERT_VALUES_EQUAL(res, expected);

        res = ExtractEntityId("минусинск");
        UNIT_ASSERT_VALUES_EQUAL(res.empty(), true);

        res = ExtractEntityId("минусинск\t0\t1");
        UNIT_ASSERT_VALUES_EQUAL(res.empty(), true);

        NJson::TJsonValue wizard;
        res = GetEntitiesString(wizard);
        UNIT_ASSERT_VALUES_EQUAL(res.empty(), true);

        wizard.SetValueByPath("rules.EntityFinder.Winner", winner);
        res = GetEntitiesString(wizard);
        UNIT_ASSERT_VALUES_EQUAL(res, expected);

        NJson::TJsonValue winnerArray;
        winnerArray.SetType(NJson::EJsonValueType::JSON_ARRAY);
        wizard.SetValueByPath("rules.EntityFinder.Winner", winnerArray);
        res = GetEntitiesString(wizard);
        UNIT_ASSERT_VALUES_EQUAL(res.empty(), true);

        winnerArray.AppendValue(winner);
        wizard.SetValueByPath("rules.EntityFinder.Winner", winnerArray);
        res = GetEntitiesString(wizard);
        UNIT_ASSERT_VALUES_EQUAL(res, expected);

        winnerArray.AppendValue(winner);
        wizard.SetValueByPath("rules.EntityFinder.Winner", winnerArray);
        res = GetEntitiesString(wizard);
        UNIT_ASSERT_VALUES_EQUAL(res, expected + "," + expected);
    }
}

} // namespace
