#include "match_success_conditions.h"

#include <contrib/libs/re2/re2/re2.h>

#include <library/cpp/json/json_reader.h>

#include <util/generic/algorithm.h>

namespace {

TString GetValueFromSlot(const NAlice::TSemanticFrame::TSlot& slot) {
    return slot.GetValue().empty() ? slot.GetTypedValue().GetString() : slot.GetValue();
}

TString GetTypeFromSlot(const NAlice::TSemanticFrame::TSlot& slot) {
    return slot.GetValue().empty() ? slot.GetTypedValue().GetType() : slot.GetType();
}

bool StringValueMatchRegex(const TString& actual, const TString& expected) {
    RE2::Options options;
    options.set_case_sensitive(false);
    const re2::RE2 pattern(expected, options);
    return pattern.ok() && RE2::PartialMatch(actual, pattern);
}

bool IsStringOrUndefinedJsonField(const NJson::TJsonValue& jsonValue) {
    return jsonValue.IsString() || jsonValue.GetType() == NJson::EJsonValueType::JSON_UNDEFINED;
}

} // namespace

namespace NAlice {

bool SlotValueSatisfiesCondition(const TString& actualValueString, const TString& expectedValueString) {
    NJson::TJsonValue actualValue;
    NJson::TJsonValue expectedValue;
    ReadJsonFastTree(actualValueString, &actualValue);
    ReadJsonFastTree(expectedValueString, &expectedValue);

    if (actualValue.IsMap() && expectedValue.IsMap()) {
        const auto checkValueItem = [&actualValue](const auto& expectedItem) -> bool {
            const auto actualItemValue = actualValue[expectedItem.first];
            const auto expectedItemValue = expectedItem.second;

            const bool stringValues = IsStringOrUndefinedJsonField(actualItemValue) && IsStringOrUndefinedJsonField(expectedItemValue);
            const bool valueMatchRegex = StringValueMatchRegex(actualItemValue.GetString(), expectedItemValue.GetString());
            const bool valuesAreEqual = expectedItemValue == actualItemValue;

            return stringValues ? valueMatchRegex : valuesAreEqual;
        };
        return AllOf(expectedValue.GetMap(), checkValueItem);
    } else {
        const bool rawValuesMatch = StringValueMatchRegex(actualValueString, expectedValueString);
        if ((IsStringOrUndefinedJsonField(actualValue) && IsStringOrUndefinedJsonField(expectedValue)) || rawValuesMatch) {
            return rawValuesMatch;
        }
        return actualValue == expectedValue;
    }
}

bool SlotSatisfiesCondition(const TSemanticFrame::TSlot& actual, const TSemanticFrame::TSlot& expected) {
    const TString actualValueString = GetValueFromSlot(actual);
    const TString expectedValueString = GetValueFromSlot(expected);

    const TString actualType = GetTypeFromSlot(actual);
    const TString expectedType = GetTypeFromSlot(expected);

    if (!expectedType.empty() && !StringValueMatchRegex(actualType, expectedType)) {
        return false;
    }
    return SlotValueSatisfiesCondition(actualValueString, expectedValueString);
}

bool FrameSatisfiesCondition(const TSemanticFrame& actual, const TSemanticFrame& expected) {
    if (!expected.GetName().empty() && expected.GetName() != actual.GetName()) {
        return false;
    }

    THashMap<TString, TSemanticFrame::TSlot> frameSlotsMap;
    for (const auto& slot : actual.GetSlots()) {
        frameSlotsMap[slot.GetName()] = slot;
    }

    for (const auto& conditionSlot : expected.GetSlots()) {
        const auto& slotName = conditionSlot.GetName();
        if (!frameSlotsMap.contains(slotName) || !SlotSatisfiesCondition(frameSlotsMap[slotName], conditionSlot)) {
            return false;
        }
    }
    return true;
}

bool AnyFrameSatisfiesCondition(const TVector<TSemanticFrame>& actualFrames, const TSemanticFrame& expected) {
    return AnyOf(actualFrames, [&expected](const TSemanticFrame& actual) {
        return FrameSatisfiesCondition(actual, expected);
    });
}

bool DeviceStateSatisfiesCondition(const TDeviceState& actualState, const TDeviceState& expectedState) {
    return !expectedState.HasIsTvPluggedIn() || actualState.GetIsTvPluggedIn() == expectedState.GetIsTvPluggedIn();
}

// Update this check if SuccessCondition matching is changed
bool IsValidSuccessCondition(const NDJ::NAS::TSuccessCondition& successCondition) {
    return successCondition.GetDeviceState().HasIsTvPluggedIn()
        || !successCondition.GetFrame().GetSlots().empty()
        || !successCondition.GetFrame().GetName().empty()
        || successCondition.HasCheck();
}

bool HasValidSuccessCondition(const NDJ::NAS::TItemAnalytics& analytics) {
    return AnyOf(analytics.GetSuccessConditions(), IsValidSuccessCondition);
}


} // NAlice
