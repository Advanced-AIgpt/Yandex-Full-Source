#include "download_info_parser.h"

#include <util/string/cast.h>
#include <alice/library/json/json.h>

namespace NAlice::NHollywood::NMusic {

namespace {

TDownloadInfoItem DownloadInfoItemFromJson(const NJson::TJsonValue& json) {
    return {
        .Codec = FromString<EAudioCodec>(json["codec"].GetStringSafe()),
        .BitrateInKbps = static_cast<i32>(json["bitrateInKbps"].GetIntegerSafe()),
        .Gain = json["gain"].GetBooleanRobust(),
        .Preview = json["preview"].GetBooleanRobust(),
        .DownloadInfoUrl = json["downloadInfoUrl"].GetStringSafe(),
        .Container = json.Has("container") ? FromString<EAudioContainer>(json["container"].GetStringSafe()) : EAudioContainer::RAW,
        .ExpiringAtMs = json["expiringAt"].GetUIntegerRobust(),
    };
}

} // namespace

TDownloadInfoOptions ParseDownloadInfo(const TStringBuf jsonString, TRTLogger& logger) {
    auto json = JsonFromString(jsonString);
    const auto& arr = json["result"].GetArraySafe();
    TDownloadInfoOptions rv;
    for (const auto& item : arr) {
        try {
            rv.push_back(DownloadInfoItemFromJson(item));
        } catch (...) {
            LOG_WARN(logger) << "Failed to parse download info item " << JsonToString(item) << ", exception:"
                             << CurrentExceptionMessage();
        }
    }
    return rv;
}

} // namespace NAlice::NHollywood::NMusic
