#include "utils.h"

#include <library/cpp/digest/md5/md5.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>


namespace NMatrix::NNotificator {

namespace {

static const TVector<std::pair<NApi::TUserDeviceInfo::ESupportedFeature, TString>> SUPPORTED_FEATURE_NAMES_VECTOR = {
    {NApi::TUserDeviceInfo::UNKNOWN, "unknown"},
    {NApi::TUserDeviceInfo::AUDIO_CLIENT, "audio_client"},
};

static const THashMap<NApi::TUserDeviceInfo::ESupportedFeature, TString> SUPPORTED_FEATURE_NAME_BY_ENUM = THashMap<NApi::TUserDeviceInfo::ESupportedFeature, TString>(
    SUPPORTED_FEATURE_NAMES_VECTOR.begin(),
    SUPPORTED_FEATURE_NAMES_VECTOR.end()
);

static const THashMap<TString, NApi::TUserDeviceInfo::ESupportedFeature> SUPPORTED_FEATURE_ENUM_BY_NAME = []() -> THashMap<TString, NApi::TUserDeviceInfo::ESupportedFeature> {
    THashMap<TString, NApi::TUserDeviceInfo::ESupportedFeature> result;
    for (const auto& [enumValue, name] : SUPPORTED_FEATURE_NAMES_VECTOR) {
        result.insert({name, enumValue});
    }

    return result;
}();

} // namespace

// As is https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/ydbs/notificator.py?rev=r8625688#L503
ui64 LegacyComputeShardKey(const TString& st) {
    const auto md5 = MD5::Calc(st);
    return IntFromString<ui64, 16>(md5.substr(md5.size() / 2));
}

TMaybe<NApi::TUserDeviceInfo::ESupportedFeature> TryParseSupportedFeatureFromString(const TString& supportedFeature) {
    if (const auto* ptr = SUPPORTED_FEATURE_ENUM_BY_NAME.FindPtr(supportedFeature)) {
        return *ptr;
    } else {
        return Nothing();
    }
}

TString SupportedFeatureToString(const NApi::TUserDeviceInfo::ESupportedFeature supportedFeature) {
    if (const auto* ptr = SUPPORTED_FEATURE_NAME_BY_ENUM.FindPtr(supportedFeature)) {
        return *ptr;
    } else {
        return "unknown";
    }
}

} // namespace NMatrix::NNotificator
