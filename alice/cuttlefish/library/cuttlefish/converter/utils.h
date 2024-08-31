#pragma once
#include <library/cpp/json/json_reader.h>
#include <util/string/ascii.h>


namespace NAlice::NCuttlefish::NAppHostServices::NConverter {

inline NJson::TJsonValue ReadJsonValue(TStringBuf raw) {
    NJson::TJsonValue value;
    NJson::ReadJsonTree(raw, &value, /*throwOnError =*/ true);
    return value;

}

template <typename ...TSteps>
inline const NJson::TJsonValue* TryGetJsonValueByPath(const NJson::TJsonValue::TMapType& root, TStringBuf step, TSteps ...steps)
{
    if (const NJson::TJsonValue* it = root.FindPtr(step)) {
        if constexpr (sizeof...(TSteps) > 0) {
            return TryGetJsonValueByPath(it->GetMapSafe(), steps...);
        }
        return it;
    }
    return nullptr;
}

template <typename ...TSteps>
inline const NJson::TJsonValue& GetJsonValueByPath(const NJson::TJsonValue::TMapType& root, TSteps ...steps)
{
    const NJson::TJsonValue* ptr = TryGetJsonValueByPath(root, steps...);
    Y_ENSURE(ptr != nullptr);
    return *ptr;
}

inline bool StartsWithCaseInsensitive(const TStringBuf a, const TStringBuf b) {
    return AsciiCompareIgnoreCase(a.Head(b.size()), b) == 0;
}

inline TString ToAsciiLower(const TStringBuf x) {
    TString str(Reserve(x.size()));
    for (char c : x)
        str.append(AsciiToLower(c));
    return str;
}

} // namespace NAlice::NCuttlefish::NAppHostServices::NConverter
