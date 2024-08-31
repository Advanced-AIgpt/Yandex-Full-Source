#include "slot.h"

#include <alice/library/json/json.h>

#include <library/cpp/json/json_value.h>

#include <util/string/join.h>

namespace NAlice {

TString GetSlotValue(const TVector<TString>& tokens, ui32 begin, ui32 end) {
    TVector<TString> valueTokens;
    for (ui32 index = begin; index < end && index < tokens.size(); ++index) {
        valueTokens.push_back(tokens[index]);
    }
    return JoinSeq(" ", valueTokens);
}

TString PackVariantsValue(const TVector<TRecognizedSlot::TVariantValue>& values) {
    NJson::TJsonValue result(NJson::EJsonValueType::JSON_ARRAY);
    for (const auto& value : values) {
        NJson::TJsonValue jsonValue;
        jsonValue[value.Type] = value.Value;
        result.AppendValue(std::move(jsonValue));
    }

    return JsonToString(result);
}

TVector<TRecognizedSlot::TVariantValue> UnPackVariantsValue(const TString& value) {
    const NJson::TJsonValue jsonValues = JsonFromString(value);
    if (!jsonValues.IsArray()) {
        return {};
    }
    TVector<TRecognizedSlot::TVariantValue> result;
    for (const auto& jsonValue : jsonValues.GetArray()) {
        if (!jsonValue.IsMap()) {
            continue;
        }
        const auto& jsonMapValue = jsonValue.GetMap();
        if (jsonMapValue.size() != 1) {
            continue;
        }
        const auto entry = jsonMapValue.begin();
        if (!entry->second.IsString()) {
            continue;
        }

        auto& value = result.emplace_back();
        value.Type = entry->first;
        value.Value = entry->second.GetString();
    }

    return result;
}

} // namespace NAlice
