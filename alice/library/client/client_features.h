#pragma once

#include "client_info.h"

#include <library/cpp/json/json_value.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/maybe.h>

namespace NAlice {

using TRawExpFlags = THashMap<TString, TMaybe<TString>>;

constexpr TStringBuf SET_ALARM_SEMANTIC_FRAME = "set_alarm_semantic_frame_v2";

class TExpFlags {
public:
    TExpFlags() = default;

    template <typename TExpCont>
    TExpFlags(const TExpCont& flags) {
        for (const auto& flag : flags) {
            Emplace(flag, TString{});
        }
    }

    TExpFlags(const NSc::TValue& flags);
    TExpFlags(const TRawExpFlags& flags);

    template <typename... Args>
    auto Emplace(Args&&... args) {
        Experiments.emplace(std::forward<Args>(args)...);
    }

    /** Check if meta experiments has given name.
     * @return true if the name exists no matter if it has value or not ("experiments": { "name1": "" } - returns true)
     */
    bool Has(TStringBuf name) const;

    /** Return value of the given experiment flag.
     * @param[in] name is a name of experiment flag
     * @return maybe is defined if the given name exists within flags and its value (can be empty)
     */
    TMaybe<TString> Value(TStringBuf name) const;

    /** Calls FN to each key from Experiments
     * @param[in] FN function that takes TStringBuf
     */
    void OnEachFlag(const std::function<void(TStringBuf)>& fn) const;

    /** Return suffix of expFlag for given prefix (return last met if there are many)
     * @param[in] expPrefix is a prefix of experiment flag
     * @return maybe is defined if the given prefix exists within flags and its suffix (can be empty)
    */
    TMaybe<TStringBuf> GetValueFromExpPrefix(const TStringBuf expPrefix) const;

    const TRawExpFlags& GetRawExpFlags() const {
        return Experiments;
    }

private:
    TRawExpFlags Experiments;
};

class TClientFeatures : public TClientInfo {
public:
    TClientFeatures(const TClientInfoProto& proto, const TRawExpFlags& flags);

    template <typename TFeaturesCont, typename TExpCont>
    TClientFeatures(
        const TClientInfoProto& proto,
        const TFeaturesCont& supportedContainer,
        const TFeaturesCont& unsupportedContainer,
        const TExpCont& expFlags
    )
        : TClientInfo{proto}
        , ExpFlags{expFlags}
    {
        for (const auto& feature : supportedContainer) {
            AddSupportedFeature(ToString(feature));
        }
        for (const auto& feature : unsupportedContainer) {
            AddUnsupportedFeature(ToString(feature));
        }
    }

    const TExpFlags& Experiments() const;

    bool HasExpFlag(const TStringBuf name) const {
        return ExpFlags.Has(name);
    }

    // trusted client features
    bool SupportsAbsoluteVolumeChange() const;
    bool SupportsAudioClient() const;
    bool SupportsAudioClientHls() const;
    bool SupportsAudioClientHlsMultiroom() const;
    bool SupportsBatteryPowerState() const;
    bool SupportsBluetoothPlayer() const;
    bool SupportsBluetoothRCU() const;
    bool SupportsBonusCardsCamera() const;
    bool SupportsBonusCardsList() const;
    bool SupportsCecAvailable() const;
    bool SupportsChangeAlarmSound() const;
    bool SupportsChangeAlarmSoundLevel() const;
    bool SupportsClockDisplay() const;
    bool SupportsCloudPush() const;
    bool SupportsCloudUi() const;
    bool SupportsCloudUiFilling() const;
    bool SupportsContentChannelAlarm() const;
    bool SupportsDarkTheme() const;
    bool SupportsDeviceLocalReminders() const;
    bool SupportsDirectiveSequencer() const;
    bool SupportsEqualizer() const;
    bool SupportsGoHomeDirective() const;
    bool SupportsIncomingMessengerCalls() const;
    bool SupportsLedDisplay() const;
    bool SupportsMapsDownloadOffline() const;
    bool SupportsMultiroom() const;
    bool SupportsMultiroomAudioClient() const;
    bool SupportsMultiroomCluster() const;
    bool SupportsMusicPlayerAllowShots() const;
    bool SupportsMusicQuasarClient() const;
    bool SupportsMuteUnmuteVolume() const;
    bool SupportsNotifications() const;
    bool SupportsOpenIBroSettings() const;
    bool SupportsOpenKeyboard() const;
    bool SupportsOpenLinkOutgoingDeviceCalls() const;
    bool SupportsOpenLinkTurboApp() const;
    bool SupportsOpenVideoEditor() const;
    bool SupportsOutgoingDeviceCalls() const;
    bool SupportsOutgoingOperatorCalls() const;
    bool SupportsPhoneAssistant() const;
    bool SupportsPlayerContinueDirective() const;
    bool SupportsPlayerDislikeDirective() const;
    bool SupportsPlayerLikeDirective() const;
    bool SupportsPlayerNextTrackDirective() const;
    bool SupportsPlayerPauseDirective() const;
    bool SupportsPlayerPreviousTrackDirective() const;
    bool SupportsPlayerRewindDirective() const;
    bool SupportsPublicAvailability() const;
    bool SupportsRelativeVolumeChange() const;
    bool SupportsRouteManagerCapability() const;
    bool SupportsS3Animations() const;
    bool SupportsScledDisplay() const; // Support 7-segments led display (mini-2)
    bool SupportsSemanticFrameAlarms() const;
    bool SupportsShowPromo() const;
    bool SupportsShowView() const;
    bool SupportsShowViewLayerContent() const;
    bool SupportsShowViewLayerFooter() const;
    bool SupportsTandemSetup() const;
    bool SupportsIotIosDeviceSetup() const;
    bool SupportsIotAndroidDeviceSetup() const;
    bool SupportsTtsPlayPlaceholder() const;
    bool SupportsTvOpenCollectionScreenDirective() const;
    bool SupportsTvOpenDetailsScreenDirective() const;
    bool SupportsTvOpenPersonScreenDirective() const;
    bool SupportsTvOpenSearchScreenDirective() const;
    bool SupportsTvOpenSeriesScreenDirective() const;
    bool SupportsUnauthorizedMusicDirectives() const;
    bool SupportsHandleAndroidAppIntent() const;
    bool SupportsTvOpenStore() const;
    bool SupportsDarkThemeSetting() const;
    bool SupportsReadSites() const;

    // not reliable features (depend on platforms and experiments)
    bool SupportsAlarms() const;
    bool SupportsAnyPlayer() const;
    bool SupportsBuiltinFeedback() const;
    bool SupportsButtons() const;
    bool SupportsCovidQrCodeLink() const;
    bool SupportsDiv2Cards() const;
    bool SupportsDivCards() const;
    bool SupportsDivCardsRendering() const;
    bool SupportsDoNotDisturbDirective() const;
    bool SupportsExternalSkillAutoClosing() const;
    bool SupportsFMRadio() const;
    bool SupportsFeedback() const;
    bool SupportsGif() const;
    bool SupportsHDMIOutput() const;
    bool SupportsImageRecognizer() const;
    bool SupportsIntentUrls() const;
    bool SupportsLiveTvScheme() const;
    bool SupportsMessaging() const;
    bool SupportsMordoviaWebview() const;
    bool SupportsMultiTabs() const;
    bool SupportsMusicPlayer() const;
    bool SupportsMusicRecognizer() const;
    bool SupportsMusicSDKPlayer() const;
    bool SupportsNavigator() const;
    bool SupportsOpenAddressBook() const;
    bool SupportsOpenLink() const;
    bool SupportsOpenLinkSearchViewport() const;
    bool SupportsOpenLinkYellowskin() const;
    bool SupportsOpenPasswordManager() const;
    bool SupportsOpenQuasarScreen() const;
    bool SupportsOpenWhocalls() const;
    bool SupportsOpenWhocallsBlocking() const;
    bool SupportsOpenWhocallsMessageFiltering() const;
    bool SupportsOpenYandexAuth() const;
    bool SupportsPedometer() const;
    bool SupportsPhoneAddressBook() const;
    bool SupportsPhoneCalls() const;
    bool SupportsReader() const;
    bool SupportsRemindersAndTodos() const;
    bool SupportsSearchFilterSet() const;
    bool SupportsServerAction() const;
    bool SupportsSoundAlarms() const;
    bool SupportsSynchronizedPush() const;
    bool SupportsTimers() const;
    bool SupportsTimersShowResponse() const;
    bool SupportsTvSourcesSwitching() const;
    bool SupportsVideoPlayDirective() const;
    bool SupportsVideoPlayer() const;
    bool SupportsVideoProtocol() const;
    bool SupportsVideoTranslationOnboarding() const;
    bool SupportsVerticalScreenNavigation() const;
    bool SupportsWhisper() const;

    // codecs
    bool SupportsVideoCodecAVC() const;
    bool SupportsVideoCodecHEVC() const;
    bool SupportsVideoCodecVP9() const;
    bool SupportsAudioBitrate192Kbps() const;
    bool SupportsAudioBitrate320Kbps() const;
    bool SupportsAudioCodecAAC() const;
    bool SupportsAudioCodecAC3() const;
    bool SupportsAudioCodecEAC3() const;
    bool SupportsAudioCodecVORBIS() const;
    bool SupportsAudioCodecOPUS() const;
    bool SupportsDynamicRangeSDR() const;
    bool SupportsDynamicRangeHDR10() const;
    bool SupportsDynamicRangeHDR10Plus() const;
    bool SupportsDynamicRangeDV() const;
    bool SupportsDynamicRangeHLG() const;

    // legacy: SK-4350
    bool SupportsNoReliableSpeakers() const;
    bool SupportsNoBluetooth() const;
    bool SupportsNoMicrophone() const;

    void ToJson(NSc::TValue* json) const;

    void AddSupportedFeature(const TString& feature);
    void AddUnsupportedFeature(const TString& feature);

private:
    bool SupportsTimersAndAlarmsAndroid() const;
    bool SupportsTimersIOS() const;
    bool SupportsAlarmsIOS() const;

    bool IsClientSupports(TStringBuf feature) const;
    bool IsClientUnsupports(TStringBuf feature) const;

protected:
    TClientFeatures(TStringBuf locale, const NSc::TValue& flags);

private:
    TExpFlags ExpFlags;
    TSet<TString> SupportedFeatures;
    TSet<TString> UnsupportedFeatures;
};

} // namespace NAlice
