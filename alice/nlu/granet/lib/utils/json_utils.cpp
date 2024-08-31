#include "json_utils.h"
#include "string_utils.h"
#include "utils.h"
#include <library/cpp/json/common/defs.h>
#include <library/cpp/json/json_reader.h>
#include <util/charset/utf8.h>
#include <util/generic/algorithm.h>
#include <util/string/ascii.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/stream/file.h>
#include <util/stream/format.h>
#include <util/stream/str.h>

namespace NGranet {

namespace NJsonUtils {

    using namespace NJson;

    void ApplyPatch(const TJsonValue& patch, TJsonValue* dest, EArrayPatchPolicy arrayPolicy) {
        Y_ENSURE(dest);
        if (patch.IsMap() && dest->IsMap()) {
            for (const auto& [key, child] : patch.GetMapSafe()) {
                ApplyPatch(child, &(*dest)[key]);
            }
            return;
        }
        if (patch.IsArray() && dest->IsArray()) {
            switch (arrayPolicy) {
                case APP_REPLACE:
                    *dest = patch;
                    break;
                case APP_MERGE:
                    for (size_t i = 0; i < patch.GetArraySafe().size(); ++i) {
                        ApplyPatch(patch[i], &(*dest)[i]);
                    }
                    break;
                case APP_EXTEND:
                    for (const TJsonValue& value : patch.GetArraySafe()) {
                        dest->AppendValue(value);
                    }
                    break;
                default:
                    Y_ENSURE(false);
            }
            return;
        }
        if (patch.IsDefined()) {
            *dest = patch;
        }
    }

    template <class TKey>
    void InsertIfDefined(TJsonValue* container, TKey key, const TJsonValue& value) {
        Y_ASSERT(container);
        if (value.IsDefined()) {
            (*container)[key] = value;
        }
    }

    TJsonValue FilterByMaskedPath(const TJsonValue& src, TStringBuf path) {
        if (path.empty() || !src.IsDefined()) {
            return src;
        }
        TJsonValue dest;
        if (path.SkipPrefix(TStringBuf("["))) {
            const TStringBuf part = path.NextTok(']');
            if (part == TStringBuf("*")) {
                for (size_t i = 0; i < src.GetArray().size(); ++i) {
                    InsertIfDefined(&dest, i, FilterByMaskedPath(src[i], path));
                }
            } else {
                const size_t i = FromString<size_t>(part);
                InsertIfDefined(&dest, i, FilterByMaskedPath(src[i], path));
            }
        } else {
            path.SkipPrefix(TStringBuf("."));
            const TStringBuf part = path.NextTokAt(path.find_first_of(TStringBuf(".[")));
            if (part == TStringBuf("*")) {
                for (const auto& [key, value] : src.GetMap()) {
                    InsertIfDefined(&dest, key, FilterByMaskedPath(value, path));
                }
            } else {
                InsertIfDefined(&dest, part, FilterByMaskedPath(src[part], path));
            }
        }
        return dest;
    }

    TJsonValue ReadJsonFileVerbose(const TFsPath& path) {
        const TString text = TFileInput(path).ReadAll();
        return ReadJsonStringVerbose(text, path);
    }

    TJsonValue ReadJsonStringVerbose(const TString& text, const TFsPath& path) {
        try {
            TStringInput input(text);
            return ReadJsonTree(&input, true);
        } catch (const TJsonException& e) {
            throw TJsonException() << PrintVerboseError(text, path, e.what());
        }
        return {};
    }

    static TStringBuf TokenBetween(TStringBuf str, TStringBuf before, TStringBuf after) {
        str.NextTok(before);
        return str.NextTok(after);
    }

    TString PrintVerboseError(const TString& text, const TFsPath& path, TStringBuf originalErrorMessage) {
        size_t offset = 0;
        if (!TryFromString<size_t>(TokenBetween(originalErrorMessage, "Offset: ", ", "), offset)
            || offset > text.length())
        {
            return TString(originalErrorMessage);
        }
        TStringBuilder out;
        out << originalErrorMessage << Endl;
        out << FormatErrorPosition(text, offset, path.GetPath());
        return out;
    }

    static bool IsKeyAllowed(TStringBuf key, const TArrayRef<const TStringBuf>& allowed) {
        for (TStringBuf pattern : allowed) {
            if (pattern.ChopSuffix("*") ? key.StartsWith(pattern) : (key == pattern)) {
                return true;
            }
        }
        return false;
    }

    void CheckAllowedKeys(const TJsonValue& map, const TArrayRef<const TStringBuf>& allowed,
        const TArrayRef<const TStringBuf>& allowedExt)
    {
        if (!map.IsDefined()) {
            return;
        }
        for (const auto& [key, value] : map.GetMapSafe()) {
            Y_ENSURE(IsKeyAllowed(key, allowed) || IsKeyAllowed(key, allowedExt), "Unknown key: " + Cite(key));
        }
    }

    TVector<const TJsonValue*> GetValues(const TJsonValue& value) {
        TVector<const TJsonValue*> result;
        if (value.IsDefined()) {
            if (value.IsArray()) {
                for (const TJsonValue& item : value.GetArraySafe()) {
                    result.push_back(&item);
                }
            } else {
                result.push_back(&value);
            }
        }
        return result;
    }

    TVector<const TJsonValue*> GetValues(const TJsonValue& map, TStringBuf key) {
        return GetValues(map[key]);
    }

    TVector<const TJsonValue*> GetValues(const TJsonValue& map, const TVector<TStringBuf>& keys) {
        TVector<const TJsonValue*> result;
        for (const TStringBuf& key : keys) {
            Extend(GetValues(map[key]), &result);
        }
        return result;
    }

    TVector<TString> GetStrings(const TJsonValue& value) {
        TVector<const TJsonValue*> values = GetValues(value);
        TVector<TString> strings(Reserve(values.size()));
        for (const TJsonValue* stringJson : values) {
            strings.push_back(stringJson->GetStringSafe());
        }
        return strings;
    }

    TVector<TString> GetStrings(const TJsonValue& map, TStringBuf key) {
        return GetStrings(map[key]);
    }

    TVector<TString> GetStrings(const TJsonValue& map, const TVector<TStringBuf>& keys) {
        TVector<TString> result;
        for (const TStringBuf& key : keys) {
            Extend(GetStrings(map[key]), &result);
        }
        return result;
    }

} // namespace NJsonUtils

} // namespace NGranet
