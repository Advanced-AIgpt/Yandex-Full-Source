#include "client_info.h"

#include <util/string/cast.h>

namespace NBASS {
namespace {

constexpr TStringBuf DEFAULT_LANG = "ru-RU";

constexpr TStringBuf OS_NAME_ANDROID = "android";
constexpr TStringBuf OS_VERSION_ANDROID_P = "p";
constexpr TStringBuf OS_VERSION_ANDROID_P_NUMERIC_FALLBACK = "8.1.99";

constexpr TStringBuf DEFAULT_USER_AGENT = "Mozilla/5.0 (Linux; Android 5.1.1; Nexus 6 Build/LYZ28E) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Mobile Safari/537.36";
constexpr TStringBuf YANDEXNAVI_USER_AGENT = "yandexnavi";

void InitFromClientId(TStringBuf clientId, NAlice::TClientInfo& info) {
    TStringBuf cl(clientId);
    // Expect data in this format:
    // https://wiki.yandex-team.ru/yandexmobile/techdocs/useragent/#format
    info.Name = cl.NextTok('/');
    info.Name.to_lower();
    info.Version = cl.NextTok(' ');
    info.SemVersion = TFullSemVer::FromString(info.Version);
    info.Manufacturer = cl.NextTok(';');
    cl.NextTok(' ');
    info.OSName = cl.NextTok(' ');
    info.OSName.to_lower();
    info.OSVersion = cl.NextTok(')');

    // Android P workaround: ASSISTANT-2161, ASSISTANT-2216
    if ((OS_NAME_ANDROID == info.OSName) && (OS_VERSION_ANDROID_P == info.OSVersion)) {
        info.OSVersion = OS_VERSION_ANDROID_P_NUMERIC_FALLBACK;
    }
}

void InitFromClientId(const TMeta& requestMetaScheme, NAlice::TClientInfo& info) {
    const auto& clientInfo = requestMetaScheme.ClientInfo();
    if (clientInfo.IsNull()) {
        InitFromClientId(requestMetaScheme.ClientId(), info);
    } else {
        info.Name = clientInfo.AppId();
        info.Name.to_lower();
        info.Version = clientInfo.AppVersion();
        info.SemVersion = TFullSemVer::FromString(info.Version);
        info.Manufacturer = clientInfo.DeviceManufacturer();
        info.OSName = clientInfo.Platform();
        info.OSName.to_lower();
        info.OSVersion = clientInfo.OsVersion();
    }
    info.Uuid = requestMetaScheme.UUID();
    info.Lang = requestMetaScheme.Lang();
    info.Epoch = requestMetaScheme.Epoch();
    info.ClientId = requestMetaScheme.ClientId();
    info.DeviceId = requestMetaScheme.DeviceId();
}

} // namespace

TClientInfo::TClientInfo(TStringBuf clientId)
    : NAlice::TClientInfo(DEFAULT_LANG)
    , UserAgent(DEFAULT_USER_AGENT)
{
    ClientId = clientId;
    ::NBASS::InitFromClientId(clientId, *this);
}


TClientInfo::TClientInfo(const TMeta& requestMetaScheme)
    : NAlice::TClientInfo(requestMetaScheme.Lang())
{
    UserAgent = requestMetaScheme.RawUserAgent();
    if (UserAgent.empty() || UserAgent == YANDEXNAVI_USER_AGENT) {
        UserAgent = DEFAULT_USER_AGENT;
    }

    ::NBASS::InitFromClientId(requestMetaScheme, *this);
}

TClientFeatures::TClientFeatures(const TMeta& requestMetaScheme, const NSc::TValue& flags)
    : NAlice::TClientFeatures(requestMetaScheme.Lang(), flags)
{
    ::NBASS::InitFromClientId(requestMetaScheme, *this);
    for (const auto& f : requestMetaScheme.ClientFeatures().Supported()) {
        AddSupportedFeature(ToString(f));
    }
    for (const auto& f : requestMetaScheme.ClientFeatures().Unsupported()) {
        AddUnsupportedFeature(ToString(f));
    }
}

} // namespace NBASS
