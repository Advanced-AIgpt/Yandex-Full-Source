#pragma once

#include <alice/library/experiments/experiments.h>
#include <alice/library/experiments/flags.h>

namespace NAlice {

/* For example, TExperimentFlagsMap could be a
    - THashMap<String, __anything__>
    - google::protobuf::Map<TProtoStringType, __anything__>
    - ...
*/
template <typename TExperimentFlagsMap>
TStringBuf GetVersion(const TExperimentFlagsMap& flags, const TStringBuf versionFlag, const TStringBuf name, const TStringBuf defaultVersion) {
    const auto versionFlagFull = TString::Join(versionFlag, name, ":");
    return GetExperimentValueWithPrefix(
        flags,
        versionFlagFull
    ).GetOrElse(defaultVersion);
}

template <typename TExperimentFlagsMap>
TStringBuf GetGifVersion(const TExperimentFlagsMap& flags, const TStringBuf name, const TStringBuf defaultVersion) {
    return GetVersion(flags, NExperiments::EXP_GIF_VERSION, name, defaultVersion);
}

inline TString FormatVersion(const TStringBuf prefix, const TStringBuf suffix, const TStringBuf version, const TStringBuf subversion, const TStringBuf sep = {}) {
    return TString::Join(prefix, sep, "v", version, ".", subversion, sep, suffix);
}

TString FormatGifVersion(const TStringBuf name, const TStringBuf suffix, const TStringBuf version, const TStringBuf subversion);

} // namespace NAlice
