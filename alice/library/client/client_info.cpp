#include "client_info.h"

#include <util/generic/strbuf.h>
#include <util/string/builder.h>
#include <util/string/cast.h>

namespace NAlice {

namespace {

bool IsQuasar(const TString& client) {
    return client.StartsWith(TStringBuf("ru.yandex.quasar"));
}

bool IsCentaur(const TString& client) {
    return client.StartsWith(TStringBuf("ru.yandex.centaur"));
}

bool IsMiniSpeaker(const TString& client) {
    return client.StartsWith(TStringBuf("aliced"));
}

NClient::EPromoType MicroPromoTypeFromColor(TStringBuf color) {
    static const THashMap<TString, NClient::EPromoType> microPromoTypes {
            {"green", NClient::PT_GREEN_PERSONALITY},
            {"tiffany", NClient::PT_GREEN_PERSONALITY},
            {"purple", NClient::PT_PURPLE_PERSONALITY},
            {"red", NClient::PT_RED_PERSONALITY},
            {"beige", NClient::PT_BEIGE_PERSONALITY},
            {"yellow", NClient::PT_YELLOW_PERSONALITY},
            {"pink", NClient::PT_PINK_PERSONALITY}
    };
    if (const auto* type = MapFindPtr(microPromoTypes, color)) {
        return *type;
    }
    return NClient::PT_NO_TYPE;
}

} // namespace

TLocale::TLocale(TStringBuf locale) {
    TStringBuf lang;
    TStringBuf country;
    // Only 'ru-RU', 'ru_RU' and 'ru' formats are supported
    if (locale.TrySplit('-', lang, country) || locale.TrySplit('_', lang, country)) {
        Lang = lang;
        Country = country;
    } else {
        Lang = locale;
    }

    // TODO Move this filter (or at least the list of supported languages) somewhere else. This is a quick fix for DIALOG-4543
    // Only 'ru' and 'tr' languages are supported
    if (Lang != TStringBuf("ru") && Lang != TStringBuf("tr")) {
        Lang = Country == TStringBuf("TR") ? TStringBuf("tr") : TStringBuf("ru");
    }

    // If country isn't given as input, it will be deduced from the language
    if (!Country) {
        // Countries for all supported languages should be here
        Country = Lang == TStringBuf("tr") ? TStringBuf("TR") : TStringBuf("RU");
    }
}

TString TLocale::ToString() const {
    if (Country.Empty()) {
        return Lang;
    }
    return TStringBuilder() << Lang << '-' << Country;
}

TClientInfo::TClientInfo(const TClientInfoProto& proto)
    : Name(proto.GetAppId())
    , Version(proto.GetAppVersion())
    , SemVersion(TFullSemVer::FromString(Version))
    , OSName(proto.GetPlatform())
    , OSVersion(proto.GetOsVersion())
    , Manufacturer(proto.GetDeviceManufacturer())
    , DeviceId(proto.GetDeviceId())
    , DeviceModel(proto.GetDeviceModel())
    , Uuid(proto.GetUuid())
    , Lang(proto.GetLang())
    , Timezone(proto.GetTimezone())
    , Epoch(FromStringWithDefault<ui64>(proto.GetEpoch(), 0))
    , Locale(proto.GetLang())
    , ClientId(ConstructClientId(proto))
    , DeviceColor(proto.GetDeviceColor())
    , PromoType(NClient::PT_NO_TYPE)
{
    Name.to_lower();
    OSName.to_lower();
    if (IsMicroSpeakerYandex()) {
        PromoType = MicroPromoTypeFromColor(DeviceColor);
    }
}

TClientInfo::TClientInfo(TStringBuf locale)
    : Locale(locale)
{
}

TStringBuf TClientInfo::GetSearchRequestUI() const {
    if (!IsTouch() && !IsSmartSpeaker())
        return TStringBuf("desktop");

    if (IsSearchApp())
        return TStringBuf("mobapp");

    // TODO (yulika@): do we need to detail the type of speaker?
    if (IsSmartSpeaker())
        return TStringBuf("quasar");

    if (IsYaBrowser())
        return TStringBuf("yabro");

    if (IsNavigator())
        return TStringBuf("navigator");

    if (IsYaAuto())
        return TStringBuf("yaauto");

    return TStringBuf("alicekit");
}

bool TClientInfo::IsChatBot() const {
    return Name == TStringBuf("telegram");
}

bool TClientInfo::IsAndroid() const {
    return (OSName == TStringBuf("android") && !IsQuasar());
}

bool TClientInfo::IsIOS() const {
    return OSName == TStringBuf("ios") ||
           OSName == TStringBuf("ipad") ||
           OSName == TStringBuf("iphone");
}

bool TClientInfo::IsWindows() const {
    return OSName == TStringBuf("windows");
}

bool TClientInfo::IsLinux() const {
    return OSName == TStringBuf("linux");
}

bool TClientInfo::IsTestClient() const {
    return Name == TStringBuf("uniproxy.monitoring") ||
           Name == TStringBuf("uniproxy.test") ||
           Name == TStringBuf("com.yandex.search.shooting") ||
           Name == TStringBuf("test") ||
           Name.StartsWith(TStringBuf("com.yandex.vins"));
}

bool TClientInfo::IsQuasar() const {
    return ::NAlice::IsQuasar(Name);
}

bool TClientInfo::IsMiniSpeaker() const {
    return ::NAlice::IsMiniSpeaker(Name);
}

bool TClientInfo::IsMiniSpeakerDexp() const {
    return IsMiniSpeaker() && Manufacturer == TStringBuf("Dexp");
}

bool TClientInfo::IsMiniSpeakerLG() const {
    return IsMiniSpeaker() && Manufacturer == TStringBuf("LG");
}

bool TClientInfo::IsMiniSpeakerYandex() const {
    return IsMiniSpeaker() && Manufacturer == TStringBuf("Yandex");
}

bool TClientInfo::IsMicroSpeakerYandex() const {
    return IsMiniSpeakerYandex() && DeviceModel == TStringBuf("yandexmicro");
}

bool TClientInfo::IsMidiSpeakerYandex() const {
    return IsMiniSpeakerYandex() && DeviceModel == TStringBuf("yandexmidi");
}

bool TClientInfo::IsSmartSpeaker() const {
    return NAlice::IsSmartSpeaker(Name);
}

bool TClientInfo::IsCentaur() const {
    return NAlice::IsCentaur(Name);
}

bool TClientInfo::IsTvDevice() const {
    return Name == TStringBuf("com.yandex.tv.alice");
}

bool TClientInfo::IsLegatus() const {
    return Name == TStringBuf("legatus");
}

bool TClientInfo::IsTestSmartSpeaker() const {
    return IsSmartSpeaker() && Name.EndsWith(TStringBuf("vins_test"));
}

bool TClientInfo::IsTouch() const {
    return IsAndroid() || IsIOS() || IsTestClient();
}

bool TClientInfo::IsDesktop() const {
    return IsYaStroka() || IsYaBrowserDesktop();
}

bool TClientInfo::IsIotApp() const {
    return Name == "com.yandex.iot" ||
           Name == "com.yandex.iot.dev" ||
           Name == "com.yandex.iot.inhouse";
}

bool TClientInfo::IsSearchAppTest() const {
    return Name == TStringBuf("ru.yandex.mobile.inhouse") ||
           Name == TStringBuf("ru.yandex.mobile.dev") ||
           Name.StartsWith(TStringBuf("ru.yandex.searchplugin.")) ||
           IsWeatherPluginTest();
}

bool TClientInfo::IsSearchAppProd() const {
    return Name == TStringBuf("ru.yandex.mobile") ||
           Name == TStringBuf("ru.yandex.searchplugin") ||
           IsWeatherPluginProd();
}

bool TClientInfo::IsSearchApp() const {
    return IsSearchAppProd() ||
           IsSearchAppTest() ||
           IsWeatherPlugin() || // absorbed client: DIALOG-5251
           IsTestClient() || // assume that all test clients are "Search app"
           IsSampleApp();
}

bool TClientInfo::IsWeatherPluginTest() const {
    return Name.StartsWith(TStringBuf("ru.yandex.weatherplugin."));
}

bool TClientInfo::IsWeatherPluginProd() const {
    return Name == TStringBuf("ru.yandex.weatherplugin");
}

bool TClientInfo::IsWeatherPlugin() const {
    return IsWeatherPluginTest() || IsWeatherPluginProd();
}

bool TClientInfo::IsWebtouch() const {
    return Name == TStringBuf("ru.yandex.webtouch");
}

// AliceKit (Android and iOS) demo applications
bool TClientInfo::IsAliceKit() const {
    return Name == TStringBuf("ru.yandex.mobile.alice") ||
           Name == TStringBuf("ru.yandex.mobile.alice.inhouse") ||
           Name == TStringBuf("ru.yandex.mobile.alice.debug") ||
           Name == TStringBuf("com.yandex.alicekit.demo");
}

bool TClientInfo::IsAliceKitTest() const {
    return Name == TStringBuf("ru.yandex.mobile.alice.inhouse")
        || Name == TStringBuf("ru.yandex.mobile.alice.debug")
        || Name.StartsWith(TStringBuf("com.yandex.alicekit.demo"));
}

bool TClientInfo::IsSampleApp() const {
    return Name == TStringBuf("com.yandex.dialog_assistant.sample") ||
           Name == TStringBuf("ru.yandex.mobile.search.dialog_assistant_sample") ||
           IsAliceKit();
}

bool TClientInfo::IsYaStroka() const {
    return Name == TStringBuf("winsearchbar");
}

bool TClientInfo::IsYaBrowserIpadTest() const {
    return Name == TStringBuf("ru.yandex.mobile.search.ipad.inhouse") ||
           Name == TStringBuf("ru.yandex.mobile.search.ipad.dev") ||
           Name == TStringBuf("ru.yandex.mobile.search.ipad.test");
}

bool TClientInfo::IsYaBrowserIpadProd() const {
    return Name == TStringBuf("ru.yandex.mobile.search.ipad");
}

bool TClientInfo::IsYaBrowserIpad() const {
    return IsYaBrowserIpadTest() ||
           IsYaBrowserIpadProd();
}

bool TClientInfo::IsYaBrowserTest() const {
    return IsYaBrowserTestDesktop() || IsYaBrowserTestMobile() || IsYaBrowserIpadTest();
}

bool TClientInfo::IsYaBrowserTestDesktop() const {
    return (
        Name == TStringBuf("yabro.beta") ||
        Name == TStringBuf("yabro.broteam") ||
        Name == TStringBuf("yabro.canary") ||
        Name == TStringBuf("yabro.dev")
    );
}

bool TClientInfo::IsYaBrowserTestMobile() const {
    return (
        // iOS
        Name == TStringBuf("ru.yandex.mobile.search.inhouse") ||
        Name == TStringBuf("ru.yandex.mobile.search.dev") ||
        Name == TStringBuf("ru.yandex.mobile.search.test") ||
        // Android
        Name == TStringBuf("com.yandex.browser.beta") ||
        Name == TStringBuf("com.yandex.browser.alpha") ||
        Name == TStringBuf("com.yandex.browser.inhouse") || // TODO: remove (ask gump@)
        Name == TStringBuf("com.yandex.browser.dev") || // TODO: remove (ask gump@)
        Name == TStringBuf("com.yandex.browser.canary") ||
        Name == TStringBuf("com.yandex.browser.broteam")
    );
}

bool TClientInfo::IsYaBrowserCanary() const {
    return IsYaBrowserCanaryDesktop() || IsYaBrowserCanaryMobile();
}

bool TClientInfo::IsYaBrowserCanaryDesktop() const {
    return Name == TStringBuf("yabro.canary");
}

bool TClientInfo::IsYaBrowserCanaryMobile() const {
    return Name == TStringBuf("com.yandex.browser.canary");
}

bool TClientInfo::IsYaBrowserProd() const {
    return IsYaBrowserProdDesktop() || IsYaBrowserProdMobile() || IsYaBrowserIpadProd();
}

bool TClientInfo::IsYaBrowserProdDesktop() const {
    return Name == TStringBuf("yabro");
}

bool TClientInfo::IsYaBrowserProdMobile() const {
    return Name == TStringBuf("ru.yandex.mobile.search") ||
           Name == TStringBuf("com.yandex.browser");
}

bool TClientInfo::IsYaBrowser() const {
    return IsYaBrowserTest() ||
           IsYaBrowserProd();
}

bool TClientInfo::IsYaBrowserDesktop() const {
    return (
        IsYaBrowserTestDesktop() ||
        IsYaBrowserCanaryDesktop() ||
        IsYaBrowserProdDesktop()
    );
}

bool TClientInfo::IsYaBrowserMobile() const {
    return (
        IsYaBrowserIpad() ||
        IsYaBrowserTestMobile() ||
        IsYaBrowserCanaryMobile() ||
        IsYaBrowserProdMobile()
    );
}

bool TClientInfo::IsYaBrowserAlphaMobile() const {
    return Name == TStringBuf("com.yandex.browser.alpha");
}

bool TClientInfo::IsYaBrowserBetaMobile() const {
    return Name == TStringBuf("com.yandex.browser.beta");
}

bool TClientInfo::IsYaLeftScreen() const {
    return Name.StartsWith(TStringBuf("ru.yandex.leftscreen")); // Android
}

bool TClientInfo::IsYaLauncher() const {
    return Name.StartsWith(TStringBuf("com.yandex.launcher")); // Android
}

bool TClientInfo::IsNavigator() const {
    return Name.StartsWith(TStringBuf("ru.yandex.mobile.navigator")) ||
           Name.StartsWith(TStringBuf("ru.yandex.yandexnavi"));
}

bool TClientInfo::IsNavigatorBeta() const {
    return Name == TStringBuf("ru.yandex.mobile.navigator.inhouse") ||
           Name == TStringBuf("ru.yandex.mobile.navigator.sandbox") ||
           Name == TStringBuf("ru.yandex.yandexnavi.inhouse") ||
           Name == TStringBuf("ru.yandex.yandexnavi.sandbox");
}

bool TClientInfo::IsYaAuto() const {
    return Name.StartsWith(TStringBuf("yandex.auto"));
}

bool TClientInfo::IsOldYaAuto() const {
    return Name.StartsWith(TStringBuf("yandex.auto.old"));
}

bool TClientInfo::IsTaximeter() const {
    return Name == TStringBuf("ru.yandex.taximeter");
}

bool TClientInfo::IsPpBeta() const {
    return Name == TStringBuf("ru.yandex.searchplugin.beta") ||
           Name == TStringBuf("ru.yandex.searchplugin.dev");
}

bool TClientInfo::IsPpNightly() const {
    return Name == TStringBuf("ru.yandex.searchplugin.nightly");
}

bool TClientInfo::IsElariWatch() const {
    return Name.StartsWith(TStringBuf("ru.yandex.iosdk.elariwatch"));
}

bool TClientInfo::IsYaMessenger() const {
    return Name == TStringBuf("ru.yandex.messenger");
}

bool TClientInfo::IsYaMusicTest() const {
    return Name.StartsWith(TStringBuf("ns.mobile.music"));
}

bool TClientInfo::IsYaMusicProd() const {
    return Name.StartsWith(TStringBuf("ru.yandex.mobile.music")) ||
           Name.StartsWith(TStringBuf("ru.yandex.music"));
}

bool TClientInfo::IsSdg() const {
    return Name.StartsWith(TStringBuf("ru.yandex.sdg"));
}

bool TClientInfo::IsYaMusic() const {
    return IsYaMusicTest() || IsYaMusicProd();
}

bool TClientInfo::IsYaModule() const {
    return DeviceModel.StartsWith(TStringBuf("YandexModule"));
}

bool TClientInfo::IsYaModule2() const {
    return DeviceModel == TStringBuf("yandexmodule_2");
}

bool TClientInfo::IsAppOfVersionOrNewer(ui16 major, ui16 minor, ui16 patch, ui16 build) const {
    return SemVersion && SemVersion->Version >= TSemVer(major, minor, patch, build);
}

bool TClientInfo::IsAndroidAppOfVersionOrNewer(ui16 major, ui16 minor, ui16 patch, ui16 build) const {
    return IsAndroid() && IsAppOfVersionOrNewer(major, minor, patch, build);
}

bool TClientInfo::IsIOSAppOfVersionOrNewer(ui16 major, ui16 minor, ui16 patch, ui16 build) const {
    return IsIOS() && IsAppOfVersionOrNewer(major, minor, patch, build);
}

bool TClientInfo::IsSystemOfVersionOrNewer(ui16 major, ui16 minor, ui16 patch, ui16 build) const {
    TMaybe<TSemVer> osSemVersion = TSemVer::FromString(OSVersion);
    return osSemVersion && osSemVersion >= TSemVer(major, minor, patch, build);
}

bool TClientInfo::IsAndroidOfVersionOrNewer(ui16 major, ui16 minor, ui16 patch, ui16 build) const {
    return IsAndroid() && IsSystemOfVersionOrNewer(major, minor, patch, build);
}

bool TClientInfo::IsIOSOfVersionOrNewer(ui16 major, ui16 minor, ui16 patch, ui16 build) const {
    return IsIOS() && IsSystemOfVersionOrNewer(major, minor, patch, build);
}

bool TClientInfo::HasScreen() const {
    return !IsSmartSpeaker() && !IsElariWatch() && !IsNavigator() && !IsYaAuto();
}

bool TClientInfo::ShouldUseDefaultWhisperConfig() const {
    // See https://st.yandex-team.ru/MEGAMIND-3226#61a6804995687b3d38b148d1 and https://st.yandex-team.ru/MEGAMIND-3165#619b5c237470490c0d63cecf
    return IsYaBrowser() || IsSearchApp() || IsYaLauncher();
}

TString SplitID(TStringBuf id) {
    if (id.size() != 32) {
        return TString{id};
    }
    TStringBuilder splittedId;
    splittedId << id.substr(0, 8) << '-'
               << id.substr(8, 4) << '-'
               << id.substr(12, 4) << '-'
               << id.substr(16, 4) << '-'
               << id.substr(20, 12);
    return splittedId;
}

bool IsSmartSpeaker(const TString& client) {
    return IsQuasar(client) || IsMiniSpeaker(client) || IsCentaur(client);
}

TString ConstructClientId(const TClientInfoProto& proto) {
    TStringBuilder clientId;
    clientId << proto.GetAppId() << "/"
             << proto.GetAppVersion() << " ("
             << proto.GetDeviceManufacturer() << " "
             << proto.GetDeviceModel() << "; "
             << proto.GetPlatform() << " "
             << proto.GetOsVersion() << ")";
    return clientId;
}

} // namespace NAlice
