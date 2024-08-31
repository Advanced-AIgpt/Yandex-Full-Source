#include "slot_inheritance.h"

#include <library/cpp/iterator/zip.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/mem.h>

namespace {
    NAlice::TOrderedFormDescription ParseJsonFormDescription(const TStringBuf& rawDescription) {
        TMemoryInput inputStream(rawDescription);
        const auto& parsedDescription = NJson::ReadJsonTree(&inputStream);
        NAlice::TOrderedFormDescription slotsDescription;
        for (const auto& slotDescription : parsedDescription.GetArray()) {
            TVector<TString> acceptedTypes;
            for (const auto& type : slotDescription.GetValueByPath("types")->GetArray()) {
                acceptedTypes.push_back(type.GetString());
            }
            slotsDescription.push_back(NAlice::TSlotDescription{slotDescription.GetValueByPath("name")->GetString(),
                                                                acceptedTypes});
        }
        return slotsDescription;
    };

    void AddSlot(
        const TString& slotName,
        const TVector<TString>& types,
        const TVector<TString>& values, NAlice::TForm* form
    ) {
        Y_ASSERT(types.size() == values.size());
        Y_ASSERT(!form->Slots.contains(slotName));
        TVector<NGranet::TResultSlotValue> slotValues;
        for (const auto& [type, value] : Zip(types, values)) {
            slotValues.push_back(NGranet::TResultSlotValue{{0, 0}, type, value});
        }
        NGranet::TResultSlot slot{{0, 0}, slotName, slotValues};
        form->Slots.emplace(slotName, slot);
    }

    void CheckFormContainsSlot(
        const NAlice::TForm& form,
        const TString& slotName,
        const TString& slotType,
        const TString& slotValue
    ) {
        UNIT_ASSERT(form.Slots.contains(slotName));
        const auto& value = form.Slots.Value(slotName, NGranet::TResultSlot{});
        UNIT_ASSERT_EQUAL(value.Name, slotName);
        UNIT_ASSERT_EQUAL(value.Data.size(), 1);
        UNIT_ASSERT_EQUAL(value.Data[0].Type, slotType);
        UNIT_ASSERT_EQUAL(value.Data[0].Value, slotValue);
    }

    NAlice::TOrderedFormDescription defaultDescritpion = ParseJsonFormDescription(R"(
        [
        {"types": ["string"], "name": "string_slot"},
        {"types": ["A", "B", "C", "string"], "name": "many_types"},
        {"types": ["C"], "name": "single_type0"},
        {"types": ["D"], "name": "single_type1"},
        {"types": ["A1", "B1"], "name": "many_types1"}
        ]
    )");
} // anonymous namespace

Y_UNIT_TEST_SUITE(TSlotReuser) {
    Y_UNIT_TEST(Simple) {
        NAlice::TForm currentForm;
        NAlice::TForm oldForm;
        AddSlot("_", {"C"}, {"valueC"}, &oldForm);
        AddSlot("many_types", {"A", "B"}, {"valueA", "valueB"}, &oldForm);
        NAlice::TSlotReuser slotReuser(oldForm, currentForm, defaultDescritpion);
        const auto& resultForms = slotReuser.Apply({});
        UNIT_ASSERT_EQUAL(resultForms.size(), 2);
        CheckFormContainsSlot(resultForms[0], "many_types", "A", "valueA");
        CheckFormContainsSlot(resultForms[0], "single_type0", "C", "valueC");
        CheckFormContainsSlot(resultForms[1], "many_types", "B", "valueB");
        CheckFormContainsSlot(resultForms[1], "single_type0", "C", "valueC");
    }

    Y_UNIT_TEST(SimpleOnlyByName) {
        NAlice::TForm currentForm;
        NAlice::TForm oldForm;
        AddSlot("_", {"C"}, {"bad_value"}, &oldForm);
        AddSlot("many_types", {"A", "B"}, {"valueA", "valueB"}, &oldForm);
        NAlice::TSlotReuser slotReuser(oldForm, currentForm, defaultDescritpion);
        NAlice::TInheritanceMode inheritanceMode;
        inheritanceMode.InheritOnlyBySlotName = true;
        const auto& resultForms = slotReuser.Apply(inheritanceMode);
        UNIT_ASSERT_EQUAL(resultForms.size(), 2);
        CheckFormContainsSlot(resultForms[0], "many_types", "A", "valueA");
        CheckFormContainsSlot(resultForms[1], "many_types", "B", "valueB");
        for (const auto& form : resultForms) {
            UNIT_ASSERT(!form.Slots.contains("single_type0"));
        }
    }

    Y_UNIT_TEST(NoOverriding) {
        NAlice::TForm currentForm;
        AddSlot("many_types", {"A"}, {"value"}, &currentForm);
        NAlice::TForm oldForm;
        AddSlot("_", {"A"}, {"bad_value"}, &oldForm);
        AddSlot("many_types", {"A", "B"}, {"bad_value_same_slot_name", "otherType"}, &oldForm);
        NAlice::TSlotReuser slotReuser(oldForm, currentForm, defaultDescritpion);
        UNIT_ASSERT_EQUAL(slotReuser.Apply({}).size(), 0);
    }

    Y_UNIT_TEST(NothingSuitable) {
        NAlice::TForm currentForm;
        NAlice::TForm oldForm;
        AddSlot("_", {"_"}, {"bad_value"}, &oldForm);
        AddSlot("many_types", {"_"}, {"bad_value_same_slot_name"}, &oldForm);
        NAlice::TSlotReuser slotReuser(oldForm, currentForm, defaultDescritpion);
        NAlice::TInheritanceMode inheritanceMode;
        UNIT_ASSERT_EQUAL(slotReuser.Apply(inheritanceMode).size(), 0);
        inheritanceMode.InheritOnlyBySlotName = true;
        UNIT_ASSERT_EQUAL(slotReuser.Apply(inheritanceMode).size(), 0);
    }

    Y_UNIT_TEST(ManyHypothesysByType) {
        NAlice::TForm currentForm;
        NAlice::TForm oldForm;
        AddSlot("0", {"A"}, {"valueA0"}, &oldForm);
        AddSlot("1", {"A"}, {"valueA1"}, &oldForm);
        AddSlot("2", {"D"}, {"valueD0"}, &oldForm);
        AddSlot("3", {"D"}, {"valueD1"}, &oldForm);
        NAlice::TSlotReuser slotReuser(oldForm, currentForm, defaultDescritpion);
        const auto& resultForms = slotReuser.Apply({});

        UNIT_ASSERT_EQUAL(resultForms.size(), 4);

        CheckFormContainsSlot(resultForms[0], "many_types", "A", "valueA1");
        CheckFormContainsSlot(resultForms[0], "single_type1", "D", "valueD1");

        CheckFormContainsSlot(resultForms[1], "many_types", "A", "valueA1");
        CheckFormContainsSlot(resultForms[1], "single_type1", "D", "valueD0");

        CheckFormContainsSlot(resultForms[2], "many_types", "A", "valueA0");
        CheckFormContainsSlot(resultForms[2], "single_type1", "D", "valueD1");

        CheckFormContainsSlot(resultForms[3], "many_types", "A", "valueA0");
        CheckFormContainsSlot(resultForms[3], "single_type1", "D", "valueD0");
    }

    Y_UNIT_TEST(ByNameHasMorePriority) {
        NAlice::TForm currentForm;
        NAlice::TForm oldForm;
        AddSlot("_", {"A"}, {"bad_value"}, &oldForm);
        AddSlot("many_types", {"A", "B"}, {"expected_value", "other_type"}, &oldForm);
        NAlice::TSlotReuser slotReuser(oldForm, currentForm, defaultDescritpion);
        const auto& resultForms = slotReuser.Apply({});
        UNIT_ASSERT_EQUAL(resultForms.size(), 2);
        CheckFormContainsSlot(resultForms[0], "many_types", "A", "expected_value");
        CheckFormContainsSlot(resultForms[1], "many_types", "B", "other_type");
    }

    Y_UNIT_TEST(NoString) {
        NAlice::TForm currentForm;
        NAlice::TForm oldForm;
        AddSlot("_", {"string"}, {"bad_value"}, &oldForm);
        AddSlot("string_slot", {"string"}, {"bad_value_same_slot_name"}, &oldForm);
        NAlice::TSlotReuser slotReuser(oldForm, currentForm, defaultDescritpion);

        UNIT_ASSERT_EQUAL(slotReuser.Apply({}).size(), 0);
    }

    Y_UNIT_TEST(SlotPrority) {
        NAlice::TForm currentForm;
        NAlice::TForm oldForm;
        AddSlot("0", {"A"}, {"valueA"}, &oldForm);
        AddSlot("1", {"B"}, {"valueB"}, &oldForm);
        AddSlot("2", {"A1"}, {"valueA1"}, &oldForm);
        AddSlot("3", {"B1"}, {"valueB1"}, &oldForm);
        NAlice::TSlotReuser slotReuser(oldForm, currentForm, defaultDescritpion);
        const auto& resultForms = slotReuser.Apply({});

        UNIT_ASSERT_EQUAL(resultForms.size(), 4);

        CheckFormContainsSlot(resultForms[0], "many_types", "A", "valueA");
        CheckFormContainsSlot(resultForms[0], "many_types1", "A1", "valueA1");
    }

    Y_UNIT_TEST(AllowSkip) {
        NAlice::TForm currentForm;
        NAlice::TForm oldForm;
        AddSlot("0", {"C"}, {"valueC"}, &oldForm);
        NAlice::TInheritanceMode inheritanceMode;
        NAlice::TSlotReuser slotReuser(oldForm, currentForm, defaultDescritpion);
        auto resultForms = slotReuser.Apply(inheritanceMode);
        UNIT_ASSERT_EQUAL(resultForms.size(), 1);
        inheritanceMode.AllowSkipSlots = true;
        resultForms = slotReuser.Apply(inheritanceMode);
        UNIT_ASSERT_EQUAL(resultForms.size(), 2);
        CheckFormContainsSlot(resultForms[0], "many_types", "C", "valueC");
        CheckFormContainsSlot(resultForms[1], "single_type0", "C", "valueC");
    }

    Y_UNIT_TEST(SkipByName) {
        NAlice::TForm currentForm;
        NAlice::TForm oldForm;
        AddSlot("many_types", {"C"}, {"valueC"}, &oldForm);
        NAlice::TInheritanceMode inheritanceMode;
        NAlice::TSlotReuser slotReuser(oldForm, currentForm, defaultDescritpion);
        auto resultForms = slotReuser.Apply(inheritanceMode);
        UNIT_ASSERT_EQUAL(resultForms.size(), 1);
        inheritanceMode.AllowSkipSlots = true;
        resultForms = slotReuser.Apply(inheritanceMode);
        UNIT_ASSERT_EQUAL(resultForms.size(), 3);
        CheckFormContainsSlot(resultForms[0], "many_types", "C", "valueC");
        CheckFormContainsSlot(resultForms[1], "many_types", "C", "valueC");
        CheckFormContainsSlot(resultForms[2], "single_type0", "C", "valueC");
    }
}
