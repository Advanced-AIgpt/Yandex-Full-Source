#pragma once

#include "fwd.h"

#include <alice/library/client/protos/client_info.pb.h>
#include <alice/library/client/protos/promo_type.pb.h>

#include <library/cpp/semver/semver.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NAlice {
/**
 * A wrapper for information about client application
 */

struct TLocale {
    TLocale(TStringBuf locale);
    TString ToString() const;

    TString Lang;
    TString Country;
};

struct TClientInfo {
    explicit TClientInfo(const TClientInfoProto& proto);

    /**
     * Get ui= cgi param value
     */
    TStringBuf GetSearchRequestUI() const;

    /**
     * Checks for OS and platforms
     */
    bool IsChatBot() const;
    bool IsAndroid() const;
    bool IsIOS() const;
    bool IsWindows() const;
    bool IsLinux() const;
    bool IsTestClient() const;
    bool IsTouch() const;
    bool IsDesktop() const;

    /**
     * Checks for Apps
     * NOTE: Using raw apps is not flexible enaugh, try use TClientFeatures
     */
    bool IsSmartSpeaker() const; // speaker as a class of devices in general
    bool IsTvDevice() const;
    bool IsLegatus() const;
    bool IsTestSmartSpeaker() const;
    bool IsQuasar() const;
    bool IsCentaur() const;
    bool IsMiniSpeaker() const;

    bool IsMiniSpeakerDexp() const;
    bool IsMiniSpeakerLG() const;
    bool IsMiniSpeakerYandex() const;
    bool IsMicroSpeakerYandex() const;
    bool IsMidiSpeakerYandex() const;

    bool IsIotApp() const;
    bool IsSearchAppTest() const;
    bool IsSearchAppProd() const;
    bool IsSearchApp() const;
    bool IsWeatherPluginTest() const;
    bool IsWeatherPluginProd() const;
    bool IsWeatherPlugin() const;
    bool IsWebtouch() const;

    bool IsAliceKit() const;
    bool IsAliceKitTest() const;
    bool IsSampleApp() const;
    bool IsYaStroka() const;

    bool IsYaBrowserIpadTest() const;
    bool IsYaBrowserIpadProd() const;
    bool IsYaBrowserIpad() const;

    bool IsYaBrowserTest() const;
    bool IsYaBrowserTestDesktop() const;
    bool IsYaBrowserTestMobile() const;

    bool IsYaBrowserCanary() const;
    bool IsYaBrowserCanaryDesktop() const;
    bool IsYaBrowserCanaryMobile() const;

    bool IsYaBrowserProd() const;
    bool IsYaBrowserProdDesktop() const;
    bool IsYaBrowserProdMobile() const;

    bool IsYaBrowser() const;
    bool IsYaBrowserDesktop() const;
    bool IsYaBrowserMobile() const;

    bool IsYaBrowserAlphaMobile() const;
    bool IsYaBrowserBetaMobile() const;

    bool IsYaLeftScreen() const;
    bool IsYaLauncher() const;
    bool IsNavigator() const;
    bool IsNavigatorBeta() const;
    bool IsYaAuto() const;
    bool IsOldYaAuto() const;

    bool IsTaximeter() const;

    bool IsPpBeta() const;
    bool IsPpNightly() const;

    bool IsElariWatch() const;

    bool IsYaMessenger() const;

    bool IsYaMusicTest() const;
    bool IsYaMusicProd() const;
    bool IsYaMusic() const;

    bool IsSdg() const;

    /* This identifies only Module + SearchApp (because Module + Smart Speaker is identified as just a Smart Speaker) */
    bool IsYaModule() const;
    bool IsYaModule2() const;

    bool IsAppOfVersionOrNewer(ui16 major, ui16 minor = 0, ui16 patch = 0, ui16 build = 0) const;
    bool IsAndroidAppOfVersionOrNewer(ui16 major, ui16 minor = 0, ui16 patch = 0, ui16 build = 0) const;
    bool IsIOSAppOfVersionOrNewer(ui16 major, ui16 minor = 0, ui16 patch = 0, ui16 build = 0) const;
    bool IsSystemOfVersionOrNewer(ui16 major, ui16 minor = 0, ui16 patch = 0, ui16 build = 0) const;
    bool IsAndroidOfVersionOrNewer(ui16 major, ui16 minor = 0, ui16 patch = 0, ui16 build = 0) const;
    bool IsIOSOfVersionOrNewer(ui16 major, ui16 minor = 0, ui16 patch = 0, ui16 build = 0) const;

    bool ShouldUseDefaultWhisperConfig() const;

    bool HasScreen() const;

    TString Name;
    TString Version;
    TMaybe<TFullSemVer> SemVersion;
    TString OSName;
    TString OSVersion;
    TString Manufacturer;
    TString DeviceId;
    TString DeviceModel;
    TString Uuid;
    TString Lang;
    TString Timezone;

    ui64 Epoch;

    TLocale Locale;

    TString ClientId;

    TString DeviceColor;
    NClient::EPromoType PromoType;

protected:
    TClientInfo(TStringBuf locale);
};

TString SplitID(TStringBuf id);

bool IsSmartSpeaker(const TString& client);

TString ConstructClientId(const TClientInfoProto& proto);

} // namespace NAlice
