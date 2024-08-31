#pragma once

#include <util/generic/fwd.h>
#include <util/generic/maybe.h>


namespace NAlice {

/* For example, TExperimentFlagsMap could be a
    - THashMap<String, __anything__>
    - google::protobuf::Map<TProtoStringType, __anything__>
    - ...
*/
template <typename TExperimentFlagsMap>
TMaybe<TStringBuf> GetExperimentValueWithPrefix(const TExperimentFlagsMap& flags, TStringBuf prefix) {
    for (const auto& [key, val] : flags) {
        TStringBuf suffix;
        if (TStringBuf{key}.AfterPrefix(prefix, suffix)) {
            return suffix;
        }
    }
    return Nothing();
}

template <typename TExperimentFlagsMap, typename T>
T GetExperimentValueWithPrefix(const TExperimentFlagsMap& flags, TStringBuf prefix, const T defaultValue) {
    T value = defaultValue;
    if (const auto expValue = GetExperimentValueWithPrefix(flags, prefix)) {
        TryFromString(*expValue, value);
    }
    return value;
}

template <typename TExperimentFlagsMap>
TMaybe<TString> GetExperimentValue(const TExperimentFlagsMap& flags, TStringBuf flag) {
    if (const auto flagPtr = flags.find(flag); flagPtr != flags.end()) {
        return flagPtr->second;
    }
    return Nothing();
}

template <typename TExperimentFlagsMap>
bool HasExpFlag(const TExperimentFlagsMap& flags, TStringBuf flag) {
    return flags.contains(flag);
}

} // namespace NAlice
