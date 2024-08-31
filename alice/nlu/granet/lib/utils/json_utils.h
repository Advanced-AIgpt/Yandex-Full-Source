#pragma once

#include <library/cpp/json/json_value.h>
#include <util/generic/array_ref.h>
#include <util/folder/path.h>

namespace NGranet {

namespace NJsonUtils {

    enum EArrayPatchPolicy {
        APP_REPLACE,
        APP_MERGE,
        APP_EXTEND,
    };

    // Apply patch to dest json.
    void ApplyPatch(const NJson::TJsonValue& patch, NJson::TJsonValue* dest, EArrayPatchPolicy arrayPolicy = APP_REPLACE);

    NJson::TJsonValue FilterByMaskedPath(const NJson::TJsonValue& src, TStringBuf path);

    // Json reading with verbose error message.
    NJson::TJsonValue ReadJsonFileVerbose(const TFsPath& path);
    NJson::TJsonValue ReadJsonStringVerbose(const TString& text, const TFsPath& path);

    // Analyze message from TJsonException and append additional information about location of error.
    TString PrintVerboseError(const TString& text, const TFsPath& path, TStringBuf originalErrorMessage);

    void CheckAllowedKeys(const NJson::TJsonValue& map, const TArrayRef<const TStringBuf>& allowed,
        const TArrayRef<const TStringBuf>& allowedExt = {});

    // If 'value' is array return its children. If 'value' is null return empty list. Otherwise return 'value'.
    TVector<const NJson::TJsonValue*> GetValues(const NJson::TJsonValue& value);
    TVector<const NJson::TJsonValue*> GetValues(const NJson::TJsonValue& map, TStringBuf key);
    TVector<const NJson::TJsonValue*> GetValues(const NJson::TJsonValue& map, const TVector<TStringBuf>& keys);
    TVector<TString> GetStrings(const NJson::TJsonValue& value);
    TVector<TString> GetStrings(const NJson::TJsonValue& map, TStringBuf key);
    TVector<TString> GetStrings(const NJson::TJsonValue& map, const TVector<TStringBuf>& keys);

    // Convert to json

    template<class T>
    NJson::TJsonValue ToJson(const T& value) {
        return NJson::TJsonValue(value);
    }

    template<class Container>
    NJson::TJsonValue ToJsonArray(const Container& container) {
        NJson::TJsonValue result(NJson::JSON_ARRAY);
        for (const auto& item : container) {
            result.AppendValue(ToJson(item));
        }
        return result;
    }

    template<class T>
    NJson::TJsonValue ToJson(const TVector<T>& container) {
        return ToJsonArray(container);
    }

} // namespace NJsonUtils

} // namespace NGranet
