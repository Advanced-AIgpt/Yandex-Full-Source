#include "client_features.h"

namespace NAlice {

namespace {

// known client features
constexpr TStringBuf ABSOLUTE_VOLUME_CHANGE = "absolute_volume_change";
constexpr TStringBuf AUDIO_BITRATE_192 = "audio_bitrate192";
constexpr TStringBuf AUDIO_BITRATE_320 = "audio_bitrate320";
constexpr TStringBuf AUDIO_CLIENT = "audio_client";
constexpr TStringBuf AUDIO_CLIENT_HLS = "audio_client_hls";
constexpr TStringBuf AUDIO_CLIENT_HLS_MULTIROOM = "audio_client_hls_multiroom";
constexpr TStringBuf AUDIO_CODEC_AAC = "audio_codec_AAC";
constexpr TStringBuf AUDIO_CODEC_AC3 = "audio_codec_AC3";
constexpr TStringBuf AUDIO_CODEC_EAC3 = "audio_codec_EAC3";
constexpr TStringBuf AUDIO_CODEC_OPUS = "audio_codec_OPUS";
constexpr TStringBuf AUDIO_CODEC_VORBIS = "audio_codec_VORBIS";
constexpr TStringBuf BATTERY_POWER_STATE = "battery_power_state";
constexpr TStringBuf BLUETOOTH_PLAYER = "bluetooth_player";
constexpr TStringBuf BLUETOOTH_RCU = "bluetooth_rcu";
constexpr TStringBuf BONUS_CARDS_CAMERA = "bonus_cards_camera";
constexpr TStringBuf BONUS_CARDS_LIST = "bonus_cards_list";
constexpr TStringBuf CEC_AVAILABLE = "cec_available";
constexpr TStringBuf CHANGE_ALARM_SOUND = "change_alarm_sound";
constexpr TStringBuf CHANGE_ALARM_SOUND_LEVEL = "change_alarm_sound_level";
constexpr TStringBuf CLOCK_DISPLAY = "clock_display";
constexpr TStringBuf CLOUD_PUSH_FEATURE = "cloud_push_implementation";
constexpr TStringBuf CLOUD_UI = "cloud_ui";
constexpr TStringBuf CLOUD_UI_FILLING = "cloud_ui_filling";
constexpr TStringBuf CONTENT_CHANNEL_ALARM = "content_channel_alarm";
constexpr TStringBuf COVID_QR = "covid_qr";
constexpr TStringBuf DARK_THEME = "dark_theme";
constexpr TStringBuf DARK_THEME_SETTING = "dark_theme_setting";
constexpr TStringBuf DEVICE_LOCAL_REMINDERS = "supports_device_local_reminders";
constexpr TStringBuf DIRECTIVE_SEQUENCER = "directive_sequencer";
constexpr TStringBuf DIV2_CARDS = "div2_cards";
constexpr TStringBuf DIV_CARDS = "div_cards";
constexpr TStringBuf DO_NOT_DISTURB = "do_not_disturb";
constexpr TStringBuf DYNAMIC_RANGE_DV = "dynamic_range_DV";
constexpr TStringBuf DYNAMIC_RANGE_HDR10 = "dynamic_range_HDR10";
constexpr TStringBuf DYNAMIC_RANGE_HDR10PLUS = "dynamic_range_HDR10Plus";
constexpr TStringBuf DYNAMIC_RANGE_HLG = "dynamic_range_HLG";
constexpr TStringBuf DYNAMIC_RANGE_SDR = "dynamic_range_SDR";
constexpr TStringBuf EQUALIZER = "equalizer";
constexpr TStringBuf GO_HOME = "go_home";
constexpr TStringBuf HANDLE_ANDROID_APP_INTENT = "handle_android_app_intent";
constexpr TStringBuf IMAGE_RECOGNIZER = "image_recognizer";
constexpr TStringBuf INCOMING_MESSENGER_CALLS_FEATURE = "incoming_messenger_calls";
constexpr TStringBuf OUTGOING_MESSENGER_CALLS_FEATURE = "outgoing_messenger_calls";
constexpr TStringBuf KEYBOARD = "keyboard";
constexpr TStringBuf LED_DISPLAY = "led_display";
constexpr TStringBuf LIVE_TV_SCHEME_FEATURE = "live_tv_scheme";
constexpr TStringBuf MAPS_DOWNLOAD_OFFLINE = "maps_download_offline";
constexpr TStringBuf MORDOVIA_WEBVIEW = "mordovia_webview";
constexpr TStringBuf MULTIROOM_AUDIO_CLIENT = "multiroom_audio_client";
constexpr TStringBuf MULTIROOM_CLUSTER_FEATURE = "multiroom_cluster";
constexpr TStringBuf MULTIROOM_FEATURE = "multiroom";
constexpr TStringBuf MUSIC_PLAYER_ALLOW_SHOTS = "music_player_allow_shots";
constexpr TStringBuf MUSIC_QUASAR_CLIENT = "music_quasar_client";
constexpr TStringBuf MUSIC_RECOGNIZER = "music_recognizer";
constexpr TStringBuf MUSIC_SDK_CLIENT = "music_sdk_client";
constexpr TStringBuf MUTE_UNMUTE_VOLUME = "mute_unmute_volume";
constexpr TStringBuf NAVIGATOR = "navigator";
constexpr TStringBuf NOTIFICATIONS = "notifications";
constexpr TStringBuf OPEN_ADDRESS_BOOK = "open_address_book";
constexpr TStringBuf OPEN_DIALOGS_IN_TABS = "open_dialogs_in_tabs";
constexpr TStringBuf OPEN_IBRO_SETTINGS = "open_ibro_settings";
constexpr TStringBuf OPEN_LINK = "open_link";
constexpr TStringBuf OPEN_LINK_INTENT = "open_link_intent";
constexpr TStringBuf OPEN_LINK_OUTGOING_DEVICE_CALLS = "open_link_outgoing_device_calls";
constexpr TStringBuf OPEN_LINK_SEARCH_VIEWPORT = "open_link_search_viewport";
constexpr TStringBuf OPEN_LINK_TURBO_APP = "open_link_turboapp";
constexpr TStringBuf OPEN_LINK_YELLOWSKIN = "open_link_yellowskin";
constexpr TStringBuf OPEN_PASSWORD_MANAGER = "pwd_app_manager";
constexpr TStringBuf OPEN_VIDEO_EDITOR = "can_open_video-editor";
constexpr TStringBuf OPEN_YANDEX_AUTH = "open_yandex_auth";
constexpr TStringBuf OUTGOING_DEVICE_CALLS = "outgoing_device_calls";
constexpr TStringBuf OUTGOING_OPERATOR_CALLS = "outgoing_operator_calls";
constexpr TStringBuf OUTGOING_PHONE_CALLS = "outgoing_phone_calls";
constexpr TStringBuf PEDOMETER = "pedometer";
constexpr TStringBuf PHONE_ADDRESS_BOOK = "phone_address_book";
constexpr TStringBuf PHONE_ASSISTANT = "phone_assistant";
constexpr TStringBuf PLAYER_CONTINUE_DIRECTIVE = "player_continue_directive";
constexpr TStringBuf PLAYER_DISLIKE_DIRECTIVE = "player_dislike_directive";
constexpr TStringBuf PLAYER_LIKE_DIRECTIVE = "player_like_directive";
constexpr TStringBuf PLAYER_NEXT_TRACK_DIRECTIVE = "player_next_track_directive";
constexpr TStringBuf PLAYER_PAUSE_DIRECTIVE = "player_pause_directive";
constexpr TStringBuf PLAYER_PREVIOUS_TRACK_DIRECTIVE = "player_previous_track_directive";
constexpr TStringBuf PLAYER_REWIND_DIRECTIVE = "player_rewind_directive";
constexpr TStringBuf PUBLICLY_AVAILABLE = "publicly_available";
constexpr TStringBuf QUASAR_SCREEN = "quasar_screen";
constexpr TStringBuf ROUTE_MANAGER_CONTINUE = "route_manager_continue";
constexpr TStringBuf ROUTE_MANAGER_SHOW = "route_manager_show";
constexpr TStringBuf ROUTE_MANAGER_START = "route_manager_start";
constexpr TStringBuf ROUTE_MANAGER_STOP = "route_manager_stop";
constexpr TStringBuf TANDEM_SETUP = "tandem_setup";
constexpr TStringBuf IOT_IOS_DEVICE_SETUP = "iot_ios_device_setup";
constexpr TStringBuf IOT_ANDROID_DEVICE_SETUP = "iot_android_device_setup";
constexpr TStringBuf READ_SITES = "can_read_sites";
constexpr TStringBuf READER_APP = "reader_app";
constexpr TStringBuf RELATIVE_VOLUME_CHANGE = "relative_volume_change";
constexpr TStringBuf S3_ANIMATIONS = "s3_animations";
constexpr TStringBuf SCLED_DISPLAY = "scled_display";
constexpr TStringBuf SERVER_ACTION = "server_action";
constexpr TStringBuf SET_ALARM_FEATURE = "set_alarm";
constexpr TStringBuf SHOW_PROMO = "show_promo";
constexpr TStringBuf SET_TIMER_FEATURE = "set_timer";
constexpr TStringBuf SHOW_TIMER = "show_timer";
constexpr TStringBuf SHOW_VIEW_FEATURE = "show_view";
constexpr TStringBuf SHOW_VIEW_LAYER_CONTENT = "show_view_layer_content";
constexpr TStringBuf SHOW_VIEW_LAYER_FOOTER = "show_view_layer_footer";
constexpr TStringBuf SYNC_PUSH_FEATURE = "synchronized_push_implementation";
constexpr TStringBuf TTS_PLAY_PLACEHOLDER = "tts_play_placeholder";
constexpr TStringBuf TV_OPEN_COLLECTION_SCREEN_DIRECTIVE = "tv_open_collection_screen_directive";
constexpr TStringBuf TV_OPEN_DETAILS_SCREEN_DIRECTIVE = "tv_open_details_screen_directive";
constexpr TStringBuf TV_OPEN_PERSON_SCREEN_DIRECTIVE = "tv_open_person_screen_directive";
constexpr TStringBuf TV_OPEN_SEARCH_SCREEN_DIRECTIVE = "tv_open_search_screen_directive";
constexpr TStringBuf TV_OPEN_SERIES_SCREEN_DIRECTIVE = "tv_open_series_screen_directive";
constexpr TStringBuf TV_OPEN_STORE = "tv_open_store";
constexpr TStringBuf UNAUTHORIZED_MUSIC_DIRECTIVES = "unauthorized_music_directives";
constexpr TStringBuf VERTICAL_SCREEN_NAVIGATION = "vertical_screen_navigation";
constexpr TStringBuf VIDEO_CODEC_AVC = "video_codec_AVC";
constexpr TStringBuf VIDEO_CODEC_HEVC = "video_codec_HEVC";
constexpr TStringBuf VIDEO_CODEC_VP9 = "video_codec_VP9";
constexpr TStringBuf VIDEO_PLAY_DIRECTIVE = "video_play_directive";
constexpr TStringBuf VIDEO_PROTOCOL_FEATURE = "video_protocol";
constexpr TStringBuf VIDEO_TRANSLATION_ONBOARDING = "can_open_videotranslation_onboarding";
constexpr TStringBuf WHOCALLS = "whocalls";
constexpr TStringBuf WHOCALLS_CALL_BLOCKING = "whocalls_call_blocking";
constexpr TStringBuf WHOCALLS_MESSAGE_FILTERING = "whocalls_message_filtering";
constexpr TStringBuf WHISPER = "whisper";

// known legacy client features: SK-4350
constexpr TStringBuf NO_BLUETOOTH = "no_bluetooth";
constexpr TStringBuf NO_MICROPHONE = "no_microphone";
constexpr TStringBuf NO_RELIABLE_SPEAKERS = "no_reliable_speakers";

// experimental flags, should be removed
constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_ALARMS = "enable_alarms";
constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_REMINDERS_AND_TODOS = "enable_reminders_todos";
constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_REMINDERS_AND_TODOS_IOS_YABRO = "enable_reminders_todos_for_ios_yabro";
constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_REMINDERS_AND_TODOS_NAVIGATOR_HIDDEN = "enable_reminders_todos_for_navigator";
constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_SOUND_ALARMS = "change_alarm_sound";
constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_TIMERS = "enable_timers";
constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_TIMERS_AND_ALARMS = "enable_timers_alarms";
constexpr TStringBuf EXPERIMENTAL_FLAG_INTERNAL_MUSIC_PLAYER = "internal_music_player";
constexpr TStringBuf EXPERIMENTAL_FLAG_MULTI_TABS = "multi_tabs";
constexpr TStringBuf EXPERIMENTAL_FLAG_RADIO_PLAY_IN_QUASAR = "radio_play_in_quasar";
constexpr TStringBuf EXPERIMENTAL_FLAG_RADIO_PLAY_IN_SEARCH = "radio_play_in_search";
constexpr TStringBuf EXPERIMENTAL_FLAG_TURN_ON_COVID_QR_FEATURE = "turn_on_covid_qr_feature";

using TMethodPtr = bool (TClientFeatures::*)() const;

struct TFeature {
    constexpr TFeature(TStringBuf flag, TMethodPtr method)
        : Flag(flag)
        , Method(method)
    {
    }

    const TStringBuf Flag;
    const TMethodPtr Method;
};

constexpr TFeature FEATURES[] = {
    {TFeature(TStringBuf("builtin_feedback"), &TClientFeatures::SupportsBuiltinFeedback)},
};

} // namespace anonymous

// TExpFlags -----------------------------------------------------------------
TExpFlags::TExpFlags(const NSc::TValue& flags) {
    if (flags.IsDict()) {
        for (const auto& kv : flags.GetDict()) {
            if (!kv.second.IsNull()) {
                Experiments.emplace(kv.first, kv.second.ForceString());
            }
        }
    } else if (flags.IsArray()) {
        for (const NSc::TValue& e : flags.GetArray()) {
            Experiments.emplace(e.GetString(), TString());
        }
    }
}

TExpFlags::TExpFlags(const THashMap<TString, TMaybe<TString>>& flags)
    : Experiments(flags)
{
}

bool TExpFlags::Has(TStringBuf flag) const {
    return Experiments.contains(flag);
}

TMaybe<TString> TExpFlags::Value(TStringBuf flag) const {
    const auto* value = Experiments.FindPtr(flag);
    if (!value) {
        return Nothing();
    }

    return *value;
}

void TExpFlags::OnEachFlag(const std::function<void(TStringBuf)>& fn) const {
    for (const auto& kv : Experiments) {
        fn(kv.first);
    }
}

TMaybe<TStringBuf> TExpFlags::GetValueFromExpPrefix(const TStringBuf expPrefix) const {
    TMaybe<TStringBuf> result;
    OnEachFlag(
        [&](auto expFlag) {
            if (expFlag.StartsWith(expPrefix)) {
                result = expFlag.SubStr(expPrefix.size());
            }
        });
    return result;
}

// TClientFeatures ------------------------------------------------------------
TClientFeatures::TClientFeatures(const TClientInfoProto& proto, const THashMap<TString, TMaybe<TString>>& flags)
    : TClientInfo(proto)
    , ExpFlags(flags)
{
}

TClientFeatures::TClientFeatures(TStringBuf locale, const NSc::TValue& flags)
    : TClientInfo(locale)
    , ExpFlags(flags)
{
}

void TClientFeatures::AddSupportedFeature(const TString& feature) {
    SupportedFeatures.emplace(feature);
}

void TClientFeatures::AddUnsupportedFeature(const TString& feature) {
    UnsupportedFeatures.emplace(feature);
}

//TODO: Add other features and think about feature-flag consistency
void TClientFeatures::ToJson(NSc::TValue* json) const {
    Y_ASSERT(json);

    NSc::TValue& features = (*json)["features"].SetDict();

    if (!SemVersion.Defined()) {
        return;
    }

    for (const auto& feature : FEATURES) {
        if ((*this.*(feature.Method))()) {
            NSc::TValue& dict = features[feature.Flag].SetDict();
            dict["enabled"].SetBool(true);
        }
    }
}

const TExpFlags& TClientFeatures::Experiments() const {
    return ExpFlags;
}

bool TClientFeatures::IsClientSupports(TStringBuf feature) const {
    return SupportedFeatures.contains(feature);
}

bool TClientFeatures::IsClientUnsupports(TStringBuf feature) const {
    return UnsupportedFeatures.contains(feature);
}

bool TClientFeatures::SupportsDivCards() const {
    if (IsClientSupports(DIV_CARDS)) {
        return true;
    } else if (IsClientUnsupports(DIV_CARDS)) {
        return false;
    }

    // legacy check, shouldn't be extended
    TMaybe<TFullSemVer> osSemVersion = TFullSemVer::FromString(OSVersion);
    TStringBuf iOSAppVersionStr = TStringBuf(Version).NextTok(':');
    int iOSAppVersion = FromString<int>(iOSAppVersionStr.data(), iOSAppVersionStr.length(), 0);

    if (IsYaAuto()) {
        return false; // coz yandex auto is android without support div cards
    }

    return (
        (IsSearchApp() &&
            ((IsAndroidAppOfVersionOrNewer(6, 60)) ||
            (IsIOS() && (osSemVersion && (osSemVersion->Version >= TSemVer(11) || iOSAppVersion > 350))))) ||
        (IsYaBrowser() && ((IsAndroidAppOfVersionOrNewer(17, 10, 1, 291)) || (IsIOS()))) ||
        (IsYaBrowserDesktop()) ||
        (IsYaStroka() && IsAppOfVersionOrNewer(1, 12)) ||
        (IsSampleApp()) ||
        (IsYaLeftScreen()) ||
        (IsYaLauncher()) ||
        (IsYaMessenger())
    );
}

bool TClientFeatures::SupportsDiv2Cards() const {
    if (IsClientSupports(DIV2_CARDS)) {
        return true;
    } else if (IsClientUnsupports(DIV2_CARDS)) {
        return false;
    }

    // legacy check, shouldn't be extended
    return IsSearchApp()
        && (IsAndroidAppOfVersionOrNewer(8, 1) || IsIOSAppOfVersionOrNewer(4, 5));
}

bool TClientFeatures::SupportsDivCardsRendering() const {
    return SupportsDivCards() || SupportsDiv2Cards();
}

bool TClientFeatures::SupportsImageRecognizer() const {
    if (IsClientSupports(IMAGE_RECOGNIZER)) {
        return true;
    } else if (IsClientUnsupports(IMAGE_RECOGNIZER)) {
        return false;
    }

    // Android versions before 5 are not supported
    if (IsAndroid() && !IsSystemOfVersionOrNewer(5)) {
        return false;
    }

    if (IsSmartSpeaker() || !SupportsDivCards()) {
        return false;
    }

    // Apps with exp flag
    if ((IsSearchAppProd() && ((IsAndroidAppOfVersionOrNewer(7, 33)) || (IsIOSAppOfVersionOrNewer(3, 75)))) ||
        ((IsYaBrowserProd() || IsYaBrowserIpadProd()) && IsIOSAppOfVersionOrNewer(18, 3, 3)) ||
        (IsYaBrowserProd() && IsAndroidAppOfVersionOrNewer(18, 3, 1, 567)) ||
        (IsYaLauncher() && IsAppOfVersionOrNewer(2, 0, 8)) ||
        (IsAliceKit())) {
        return true;
    }

    // Apps without exp flag
    if ((IsSearchAppTest() && (IsAndroidAppOfVersionOrNewer(7, 33) || IsIOSAppOfVersionOrNewer(3, 75))) ||
        (IsYaBrowserTest() && (IsAndroidAppOfVersionOrNewer(18, 3, 1, 567) || IsIOSAppOfVersionOrNewer(18, 3, 3))) ||
        (IsYaBrowserCanary() && IsAppOfVersionOrNewer(18, 3, 1)) ||
        (IsSampleApp())) {
        return true;
    }

    return false;
}

bool TClientFeatures::SupportsIntentUrls() const {
    if (IsClientSupports(OPEN_LINK_INTENT)) {
        return true;
    } else if (IsClientUnsupports(OPEN_LINK_INTENT)) {
        return false;
    }

    // legacy check, shouldn't be extended
    return IsSearchApp() ||
        (Name == TStringBuf("ru.yandex.mobile.search.inhouse")) ||
        (IsYaBrowser() && IsAndroidAppOfVersionOrNewer(17, 10, 1, 291)) ||
        (IsYaBrowser() && IsIOS()) ||
        (IsYaLauncher()) ||
        (IsNavigator());
}

bool TClientFeatures::SupportsMusicRecognizer() const {
    if (IsClientSupports(MUSIC_RECOGNIZER)) {
        return true;
    } else if (IsClientUnsupports(MUSIC_RECOGNIZER)) {
        return false;
    }

    // legacy check, shouldn't be extended

    if (IsClientSupports(NO_MICROPHONE)) {
        return false;
    }

    if ((IsSearchApp() && (IsAndroidAppOfVersionOrNewer(7, 33) || IsIOSAppOfVersionOrNewer(3, 75))) ||
        ((IsYaBrowser() || IsYaBrowserIpad()) && IsIOSAppOfVersionOrNewer(18, 3, 3)) ||
        (IsYaBrowser() && IsAndroidAppOfVersionOrNewer(18, 3, 1, 567)) ||
        (IsYaLauncher() && IsAppOfVersionOrNewer(2, 0, 8)) ||
        (IsSmartSpeaker()) ||
        (IsYaMusic()) ||
        (IsNavigator() && (IsAndroidAppOfVersionOrNewer(3, 95) || IsIOSAppOfVersionOrNewer(395))) ||
        (IsAliceKit()) ||
        (IsSampleApp()) ||
        (IsTestClient())
    ) {
        return true;
    }

    return false;
}

bool TClientFeatures::SupportsPhoneCalls() const {
    if (IsClientSupports(OUTGOING_PHONE_CALLS)) {
        return true;
    } else if (IsClientUnsupports(OUTGOING_PHONE_CALLS)) {
        return false;
    }

    if (IsYaAuto() || IsElariWatch()) {
        return false; // coz yandex auto and elari watch is android without phone calls
    }

    return IsAndroid() || IsIOS() || IsTestClient();
}

bool TClientFeatures::SupportsPhoneAddressBook() const {
    return IsClientSupports(PHONE_ADDRESS_BOOK);
}

bool TClientFeatures::SupportsOpenAddressBook() const {
    if (IsClientSupports(OPEN_ADDRESS_BOOK)) {
        return true;
    } else if (IsClientUnsupports(OPEN_ADDRESS_BOOK)) {
        return false;
    }

    if (IsYaAuto() || IsElariWatch()) {
        return false; // coz yandex auto and elari watch is android without phone calls
    }

    return IsAndroid();
}

bool TClientFeatures::SupportsReader() const {
    if (IsClientSupports(READER_APP)) {
        return true;
    } else if (IsClientUnsupports(READER_APP)) {
        return false;
    }
    return false;
}

bool TClientFeatures::SupportsSearchFilterSet() const {
    if (IsTestClient() || IsChatBot()) {
        return true;
    }
    if (IsSearchApp()) {
        return IsAndroidAppOfVersionOrNewer(8, 70);
    }
    return false;
}

bool TClientFeatures::SupportsBuiltinFeedback() const {
    return IsAliceKitTest() ||
        ((IsPpBeta() || IsPpNightly()) && IsAppOfVersionOrNewer(7, 90)) ||
        (IsSearchAppProd() && IsAndroidAppOfVersionOrNewer(7, 90)) ||
        (IsYaBrowserTestMobile() && IsAndroidAppOfVersionOrNewer(18, 11)) ||
        (IsYaBrowserProdMobile() && IsAndroidAppOfVersionOrNewer(18, 11));
}

bool TClientFeatures::SupportsOpenYandexAuth() const {
    if (IsClientSupports(OPEN_YANDEX_AUTH)) {
        return true;
    } else if (IsClientUnsupports(OPEN_YANDEX_AUTH)) {
        return false;
    }

    return IsSearchApp();
}

bool TClientFeatures::SupportsPedometer() const {
    return IsClientSupports(PEDOMETER);
}

bool TClientFeatures::SupportsButtons() const {
    return !IsNavigator();
}

bool TClientFeatures::SupportsCovidQrCodeLink() const {
    if (IsClientSupports(COVID_QR)) {
        return true;
    } else if (IsClientUnsupports(COVID_QR)) {
        return false;
    }
    return ExpFlags.Has(EXPERIMENTAL_FLAG_TURN_ON_COVID_QR_FEATURE);
}

bool TClientFeatures::SupportsExternalSkillAutoClosing() const {
    return (IsAndroid()
            && (IsSearchApp() && IsAppOfVersionOrNewer(7, 85, 0)
            || IsYaBrowserMobile() && IsAppOfVersionOrNewer(18, 10, 0)
            || IsYaLauncher() && IsAppOfVersionOrNewer(2, 2, 2)))
        || (IsIOS()
            && (IsSearchApp() && IsAppOfVersionOrNewer(4, 30, 0)
            || IsYaBrowserMobile() && IsAppOfVersionOrNewer(18, 11, 4)))
        || IsAliceKitTest();
}

bool TClientFeatures::SupportsMultiTabs() const {
    if (IsClientSupports(OPEN_DIALOGS_IN_TABS)) {
        return true;
    } else if (IsClientUnsupports(OPEN_DIALOGS_IN_TABS)) {
        return false;
    }
    // legacy check, shouldn't be extended
    if (ExpFlags.Has(EXPERIMENTAL_FLAG_MULTI_TABS)) {
        return true;
    }

    if (!SemVersion) {
        return false;
    }

    const TSemVer& ver = SemVersion->Version;
    const bool isAndroid = IsAndroid();
    const bool isIOS = IsIOS();

    if (IsYaBrowser()) {
        constexpr TSemVer androidVer{18, 4, 1, 0};
        constexpr TSemVer iosVer{18, 4, 3, 90};

        return (isAndroid && ver >= androidVer)
            || (isIOS && ver >= iosVer);
    }

    // It is important that sample app must be before searchapp
    // because searchapp IS sampleapp.
    if (IsSampleApp()) {
        constexpr TSemVer androidVer{1, 4, 0};
        constexpr TSemVer iosVer{1, 4, 0};

        return (isAndroid && ver >= androidVer)
            || (isIOS && ver >= iosVer);
    }

    if (IsSearchApp()) {
        constexpr TSemVer androidVer{7, 50};
        constexpr TSemVer iosVer{3, 90};

        return (isAndroid && ver >= androidVer)
            || (isIOS && ver >= iosVer);
    }

    if (IsYaLauncher()) {
        constexpr TSemVer allVer{2, 1, 2};
        return ver >= allVer;
    }

    return false;
}

bool TClientFeatures::SupportsHDMIOutput() const {
    return IsQuasar();
}

bool TClientFeatures::SupportsMusicPlayer() const {
    if (SupportsMusicSDKPlayer() || SupportsAudioClient() || SupportsMusicQuasarClient()) {
        return true;
    }

    // legacy check, shouldn't be extended
    return IsSmartSpeaker();
}

bool TClientFeatures::SupportsMusicSDKPlayer() const {
    if (IsClientSupports(MUSIC_SDK_CLIENT)) {
        return true;
    } else if (IsClientUnsupports(MUSIC_SDK_CLIENT)) {
        return false;
    }

    if (IsYaMusic()) {
        return true;
    }

    if (!ExpFlags.Has(EXPERIMENTAL_FLAG_INTERNAL_MUSIC_PLAYER)) {
        return false;
    }

    if (IsNavigator()) {
        return true;
    }

    if (IsIOS() && (IsSearchApp() || IsAliceKit())) {
        return true;
    }

    if (!IsAndroidOfVersionOrNewer(5)) {
        return false;
    }

    return (IsSearchApp() && IsAppOfVersionOrNewer(8, 7))
        || IsAliceKit() && IsAppOfVersionOrNewer(6, 0)
        || IsYaBrowserTestMobile() && IsAppOfVersionOrNewer(19, 3, 0)
        || IsAliceKitTest();
}

bool TClientFeatures::SupportsNavigator() const {
    if (IsClientSupports(NAVIGATOR)) {
        return true;
    } else if (IsClientUnsupports(NAVIGATOR)) {
        return false;
    }

    // legacy check, shouldn't be extended
    return IsNavigator() || IsYaAuto();;
}

bool TClientFeatures::SupportsBluetoothPlayer() const {
    if (IsClientSupports(BLUETOOTH_PLAYER)) {
        return true;
    } else if (IsClientUnsupports(BLUETOOTH_PLAYER)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsBluetoothRCU() const {
    if (IsClientSupports(BLUETOOTH_RCU)) {
        return true;
    } else if (IsClientUnsupports(BLUETOOTH_RCU)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsBonusCardsCamera() const {
    return IsClientSupports(BONUS_CARDS_CAMERA);
}

bool TClientFeatures::SupportsBonusCardsList() const {
    return IsClientSupports(BONUS_CARDS_LIST);
}

bool TClientFeatures::SupportsVideoPlayer() const {
    return IsSmartSpeaker() || IsTvDevice();
}

bool TClientFeatures::SupportsAnyPlayer() const {
    return SupportsMusicPlayer() || SupportsBluetoothPlayer() || SupportsFMRadio() || SupportsVideoPlayer();
}

bool TClientFeatures::SupportsFMRadio() const {
    return
        (ExpFlags.Has(EXPERIMENTAL_FLAG_RADIO_PLAY_IN_QUASAR) && IsSmartSpeaker()) ||
        (ExpFlags.Has(EXPERIMENTAL_FLAG_RADIO_PLAY_IN_SEARCH) && (IsSearchApp() || IsDesktop() || IsYaBrowserMobile() || IsYaLauncher() || IsNavigator() || IsNavigatorBeta()));
}

bool TClientFeatures::SupportsFeedback() const {
    return !IsSmartSpeaker() && !IsTvDevice() && !IsNavigator() && !IsYaAuto() && !IsLegatus();
}

bool TClientFeatures::SupportsMessaging() const {
    return IsAliceKitTest() && IsAndroidOfVersionOrNewer(6);
}

bool TClientFeatures::SupportsVideoPlayDirective() const {
    if (IsClientSupports(VIDEO_PLAY_DIRECTIVE)) {
        return true;
    } else if (IsClientUnsupports(VIDEO_PLAY_DIRECTIVE)) {
        return false;
    }
    // legacy check, shouldn't be extended
    return IsSmartSpeaker();
}

bool TClientFeatures::SupportsVideoProtocol() const {
    if (IsClientSupports(VIDEO_PROTOCOL_FEATURE)) {
        return true;
    } else if (IsClientUnsupports(VIDEO_PROTOCOL_FEATURE)) {
        return false;
    }

    // legacy check, shouldn't be extended
    return IsSmartSpeaker();
}

bool TClientFeatures::SupportsVideoTranslationOnboarding() const {
    return IsClientSupports(VIDEO_TRANSLATION_ONBOARDING);
}

bool TClientFeatures::SupportsLiveTvScheme() const {
    return IsClientSupports(LIVE_TV_SCHEME_FEATURE);
}

bool TClientFeatures::SupportsTvSourcesSwitching() const {
    return IsTvDevice() || IsAliceKitTest();
}

bool TClientFeatures::SupportsRemindersAndTodos() const {
    return (
        ExpFlags.Has(EXPERIMENTAL_FLAG_ENABLE_REMINDERS_AND_TODOS) && (
            IsSmartSpeaker()
            || IsYaLauncher() && IsAppOfVersionOrNewer(2, 2, 1)
            || IsAliceKitTest() && (IsAndroid() || IsIOSAppOfVersionOrNewer(0, 1))
            || (IsSearchAppTest() || IsSearchAppProd()) && !IsWeatherPlugin() &&
               (IsAndroidAppOfVersionOrNewer(7, 40) || IsIOSAppOfVersionOrNewer(400))
            || (IsYaBrowserTestMobile() || IsYaBrowserProdMobile()) && IsAndroidAppOfVersionOrNewer(18, 3, 1, 586)
        )
        || ExpFlags.Has(EXPERIMENTAL_FLAG_ENABLE_REMINDERS_AND_TODOS_IOS_YABRO) &&
           (IsYaBrowserTestMobile() || IsYaBrowserProdMobile()) && IsIOSAppOfVersionOrNewer(19, 1, 1)
        || ExpFlags.Has(EXPERIMENTAL_FLAG_ENABLE_REMINDERS_AND_TODOS_NAVIGATOR_HIDDEN) &&
           IsNavigator() && (IsAndroid() || IsIOS())
        || SupportsCloudPush()
    );
}

// Doesn't check any flags or OS version
bool TClientFeatures::SupportsTimersAndAlarmsAndroid() const {
    if (!IsAndroid()) {
        return false;
    }
    return IsAliceKitTest()
        || IsYaBrowserMobile() && IsAppOfVersionOrNewer(18, 3, 1, 586)
        || (IsSearchAppTest() || IsSearchAppProd()) && IsAppOfVersionOrNewer(7, 33)
        || IsYaLauncher() && IsAppOfVersionOrNewer(2, 0, 8)
        || IsNavigator() && IsAppOfVersionOrNewer(4);
}

// Doesn't check any flags or OS version
bool TClientFeatures::SupportsTimersIOS() const {
    if (!IsIOS()) {
        return false;
    }
    return IsAliceKitTest()
        || (IsSearchAppTest() || IsSearchAppProd()) && !IsWeatherPlugin() && IsAppOfVersionOrNewer(420)
        || IsNavigator() && IsAppOfVersionOrNewer(400);
}

// Doesn't check any flags or OS version
bool TClientFeatures::SupportsAlarmsIOS() const {
    if (!IsIOS()) {
        return false;
    }
    return IsAliceKitTest();
}

bool TClientFeatures::SupportsServerAction() const {
    if (IsClientSupports(SERVER_ACTION)) {
        return true;
    } else if (IsClientUnsupports(SERVER_ACTION)) {
        return false;
    }
    // legacy check, shouldn't be extended
    return IsSmartSpeaker() || IsSearchApp();
}

bool TClientFeatures::SupportsAlarms() const {
    if (IsClientSupports(SET_ALARM_FEATURE)) {
        return true;
    } else if (IsClientUnsupports(SET_ALARM_FEATURE)) {
        return false;
    }

    // legacy check, shouldn't be extended
    if (!ExpFlags.Has(EXPERIMENTAL_FLAG_ENABLE_ALARMS) &&
        (!ExpFlags.Has(EXPERIMENTAL_FLAG_ENABLE_TIMERS_AND_ALARMS) || IsIOS())
    ) {
        return false;
    }
    return IsSmartSpeaker()
        || SupportsTimersAndAlarmsAndroid() && IsAndroidOfVersionOrNewer(4, 4)
        || SupportsAlarmsIOS() && IsIOSOfVersionOrNewer(10);
}

bool TClientFeatures::SupportsContentChannelAlarm() const {
    if (IsClientSupports(CONTENT_CHANNEL_ALARM)) {
        return true;
    } else if (IsClientUnsupports(CONTENT_CHANNEL_ALARM)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsSoundAlarms() const {
    return ExpFlags.Has(EXPERIMENTAL_FLAG_ENABLE_SOUND_ALARMS) && IsSmartSpeaker() && SupportsAlarms();
}

bool TClientFeatures::SupportsSemanticFrameAlarms() const {
    return IsClientSupports(SET_ALARM_SEMANTIC_FRAME);
}

bool TClientFeatures::SupportsGif() const {
    return ((IsSearchApp() && (IsAndroidAppOfVersionOrNewer(9, 30) || IsIOSAppOfVersionOrNewer(1800))) ||
                (IsYaBrowserMobile() && (IsAndroidAppOfVersionOrNewer(20, 3) || IsIOSAppOfVersionOrNewer(2002))) ||
                (IsYaLauncher() && IsAndroidAppOfVersionOrNewer(2, 3, 6))) &&
            (IsIOS() || IsAndroidOfVersionOrNewer(9u));
}

bool TClientFeatures::SupportsMultiroom() const {
    return IsClientSupports(MULTIROOM_FEATURE);
}

bool TClientFeatures::SupportsMultiroomAudioClient() const {
    return IsClientSupports(MULTIROOM_AUDIO_CLIENT);
}

bool TClientFeatures::SupportsMultiroomCluster() const {
    return IsClientSupports(MULTIROOM_CLUSTER_FEATURE);
}

bool TClientFeatures::SupportsMuteUnmuteVolume() const {
    if (IsClientSupports(MUTE_UNMUTE_VOLUME)) {
        return true;
    } else if (IsClientUnsupports(MUTE_UNMUTE_VOLUME)) {
        return false;
    }

    // default
    return true;
}

bool TClientFeatures::SupportsIncomingMessengerCalls() const {
    return IsClientSupports(INCOMING_MESSENGER_CALLS_FEATURE);
}

bool TClientFeatures::SupportsTimers() const {
    if (IsClientSupports(SET_TIMER_FEATURE)) {
        return true;
    } else if (IsClientUnsupports(SET_TIMER_FEATURE)) {
        return false;
    }

    // legacy check, shouldn't be extended
    if (!ExpFlags.Has(EXPERIMENTAL_FLAG_ENABLE_TIMERS) &&
        (!ExpFlags.Has(EXPERIMENTAL_FLAG_ENABLE_TIMERS_AND_ALARMS) || IsIOS())
    ) {
        return false;
    }
    return IsSmartSpeaker()
        || SupportsTimersAndAlarmsAndroid() && IsAndroidOfVersionOrNewer(4, 4)
        || SupportsTimersIOS() && IsIOSOfVersionOrNewer(10);
}

bool TClientFeatures::SupportsTimersShowResponse() const {
    if (IsClientSupports(SHOW_TIMER)) {
        return true;
    } else if (IsClientUnsupports(SHOW_TIMER)) {
        return false;
    }

    // legacy check, shouldn't be extended
    return SupportsTimersAndAlarmsAndroid() && IsAndroidOfVersionOrNewer(8)
        || SupportsTimersIOS() && IsIOSOfVersionOrNewer(10);
}

bool TClientFeatures::SupportsShowView() const {
    if (IsClientUnsupports(SHOW_VIEW_FEATURE)) {
        return false;
    } else if (IsClientSupports(SHOW_VIEW_FEATURE)) {
        return true;
    }

    return false;
}

bool TClientFeatures::SupportsShowViewLayerContent() const {
    if (IsClientUnsupports(SHOW_VIEW_LAYER_CONTENT)) {
        return false;
    } else if (IsClientSupports(SHOW_VIEW_LAYER_CONTENT)) {
        return true;
    }
    // default
    return SupportsShowView();
}

bool TClientFeatures::SupportsShowViewLayerFooter() const {
    if (IsClientUnsupports(SHOW_VIEW_LAYER_FOOTER)) {
        return false;
    } else if (IsClientSupports(SHOW_VIEW_LAYER_FOOTER)) {
        return true;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsSynchronizedPush() const {
    if (IsClientUnsupports(SYNC_PUSH_FEATURE)) {
        return false;
    } else if (IsClientSupports(SYNC_PUSH_FEATURE)) {
        return true;
    }
    // legacy check, shouldn't be extended
    return IsSmartSpeaker();
}

bool TClientFeatures::SupportsLedDisplay() const {
    if (IsClientSupports(LED_DISPLAY)) {
        return true;
    } else if (IsClientUnsupports(LED_DISPLAY)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsS3Animations() const {
    return IsClientSupports(S3_ANIMATIONS);
}

bool TClientFeatures::SupportsScledDisplay() const {
    return IsClientSupports(SCLED_DISPLAY);
}

bool TClientFeatures::SupportsMapsDownloadOffline() const {
    if (IsClientSupports(MAPS_DOWNLOAD_OFFLINE)) {
        return true;
    } else if (IsClientUnsupports(MAPS_DOWNLOAD_OFFLINE)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsMordoviaWebview() const {
    if (IsClientSupports(MORDOVIA_WEBVIEW)) {
        return true;
    } else if (IsClientUnsupports(MORDOVIA_WEBVIEW)) {
        return false;
    }
    // legacy check, shouldn't be extended
    return IsYaModule();
}

bool TClientFeatures::SupportsAudioClient() const {
    if (IsClientSupports(AUDIO_CLIENT)) {
        return true;
    } else if (IsClientUnsupports(AUDIO_CLIENT)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsAudioClientHls() const {
    if (IsClientSupports(AUDIO_CLIENT_HLS)) {
        return true;
    } else if (IsClientUnsupports(AUDIO_CLIENT_HLS)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsAudioClientHlsMultiroom() const {
    return IsClientSupports(AUDIO_CLIENT_HLS_MULTIROOM);
}

bool TClientFeatures::SupportsMusicQuasarClient() const {
    if (IsClientSupports(MUSIC_QUASAR_CLIENT)) {
        return true;
    } else if (IsClientUnsupports(MUSIC_QUASAR_CLIENT)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsBatteryPowerState() const {
    if (IsClientSupports(BATTERY_POWER_STATE)) {
        return true;
    } else if (IsClientUnsupports(BATTERY_POWER_STATE)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsCecAvailable() const {
    if (IsClientSupports(CEC_AVAILABLE)) {
        return true;
    } else if (IsClientUnsupports(CEC_AVAILABLE)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsChangeAlarmSound() const {
    if (IsClientSupports(CHANGE_ALARM_SOUND)) {
        return true;
    } else if (IsClientUnsupports(CHANGE_ALARM_SOUND)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsChangeAlarmSoundLevel() const {
    if (IsClientSupports(CHANGE_ALARM_SOUND_LEVEL)) {
        return true;
    } else if (IsClientUnsupports(CHANGE_ALARM_SOUND_LEVEL)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsClockDisplay() const {
    if (IsClientSupports(CLOCK_DISPLAY)) {
        return true;
    } else if (IsClientUnsupports(CLOCK_DISPLAY)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsCloudPush() const {
    if (IsClientSupports(CLOUD_PUSH_FEATURE)) {
        return true;
    } else if (IsClientUnsupports(CLOUD_PUSH_FEATURE)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsCloudUi() const {
    if (IsClientSupports(CLOUD_UI)) {
        return true;
    } else if (IsClientUnsupports(CLOUD_UI)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsCloudUiFilling() const {
    if (IsClientSupports(CLOUD_UI_FILLING)) {
        return true;
    } else if (IsClientUnsupports(CLOUD_UI_FILLING)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsDarkTheme() const {
    return IsClientSupports(DARK_THEME);
}

bool TClientFeatures::SupportsDeviceLocalReminders() const {
    return IsClientSupports(DEVICE_LOCAL_REMINDERS);
}

bool TClientFeatures::SupportsDirectiveSequencer() const {
    if (IsClientSupports(DIRECTIVE_SEQUENCER)) {
        return true;
    } else if (IsClientUnsupports(DIRECTIVE_SEQUENCER)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsEqualizer() const {
    return IsClientSupports(EQUALIZER);
}

bool TClientFeatures::SupportsMusicPlayerAllowShots() const {
    if (IsClientSupports(MUSIC_PLAYER_ALLOW_SHOTS)) {
        return true;
    } else if (IsClientUnsupports(MUSIC_PLAYER_ALLOW_SHOTS)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsTandemSetup() const {
    return IsClientSupports(TANDEM_SETUP);
}

bool TClientFeatures::SupportsIotIosDeviceSetup() const {
    return IsClientSupports(IOT_IOS_DEVICE_SETUP);
}

bool TClientFeatures::SupportsIotAndroidDeviceSetup() const {
    return IsClientSupports(IOT_ANDROID_DEVICE_SETUP);
}

bool TClientFeatures::SupportsTtsPlayPlaceholder() const {
    if (IsClientSupports(TTS_PLAY_PLACEHOLDER)) {
        return true;
    } else if (IsClientUnsupports(TTS_PLAY_PLACEHOLDER)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsTvOpenCollectionScreenDirective() const {
    if (IsClientSupports(TV_OPEN_COLLECTION_SCREEN_DIRECTIVE)) {
        return true;
    } else if (IsClientUnsupports(TV_OPEN_COLLECTION_SCREEN_DIRECTIVE)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsTvOpenDetailsScreenDirective() const {
    if (IsClientSupports(TV_OPEN_DETAILS_SCREEN_DIRECTIVE)) {
        return true;
    } else if (IsClientUnsupports(TV_OPEN_DETAILS_SCREEN_DIRECTIVE)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsTvOpenPersonScreenDirective() const {
    if (IsClientSupports(TV_OPEN_PERSON_SCREEN_DIRECTIVE)) {
        return true;
    } else if (IsClientUnsupports(TV_OPEN_PERSON_SCREEN_DIRECTIVE)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsTvOpenSearchScreenDirective() const {
    if (IsClientSupports(TV_OPEN_SEARCH_SCREEN_DIRECTIVE)) {
        return true;
    } else if (IsClientUnsupports(TV_OPEN_SEARCH_SCREEN_DIRECTIVE)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsTvOpenSeriesScreenDirective() const {
    if (IsClientSupports(TV_OPEN_SERIES_SCREEN_DIRECTIVE)) {
        return true;
    } else if (IsClientUnsupports(TV_OPEN_SERIES_SCREEN_DIRECTIVE)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsUnauthorizedMusicDirectives() const {
    if (IsClientSupports(UNAUTHORIZED_MUSIC_DIRECTIVES)) {
        return true;
    } else if (IsClientUnsupports(UNAUTHORIZED_MUSIC_DIRECTIVES)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsVerticalScreenNavigation() const {
    if (IsClientSupports(VERTICAL_SCREEN_NAVIGATION)) {
        return true;
    } else if (IsClientUnsupports(VERTICAL_SCREEN_NAVIGATION)) {
        return false;
    }
    // default
    return SupportsMordoviaWebview();
}

bool TClientFeatures::SupportsNotifications() const {
    if (IsClientSupports(NOTIFICATIONS)) {
        return true;
    } else if (IsClientUnsupports(NOTIFICATIONS)) {
        return false;
    }
    // legacy check, shouldn't be extended
    return false;
}

bool TClientFeatures::SupportsOpenIBroSettings() const {
    if (IsClientSupports(OPEN_IBRO_SETTINGS)) {
        return true;
    } else if (IsClientUnsupports(OPEN_IBRO_SETTINGS)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsOpenKeyboard() const {
    if (IsClientSupports(KEYBOARD)) {
        return true;
    } else if (IsClientUnsupports(KEYBOARD)) {
        return false;
    }
    // legacy check, shouldn't be extended
    return false;
}

bool TClientFeatures::SupportsOpenLink() const {
    if (IsClientSupports(OPEN_LINK)) {
        return true;
    } else if (IsClientUnsupports(OPEN_LINK)) {
        return false;
    }
    // legacy check, shouldn't be extended
    return !IsSmartSpeaker() && !IsYaAuto() && !IsElariWatch();
}

bool TClientFeatures::SupportsOpenLinkSearchViewport() const {
    if (IsClientSupports(OPEN_LINK_SEARCH_VIEWPORT)) {
        return true;
    } else if (IsClientUnsupports(OPEN_LINK_SEARCH_VIEWPORT)) {
        return false;
    }
    // legacy check, shouldn't be extended
    return IsSearchApp();
}

bool TClientFeatures::SupportsOpenLinkTurboApp() const {
    if (IsClientSupports(OPEN_LINK_TURBO_APP)) {
        return true;
    } else if (IsClientUnsupports(OPEN_LINK_TURBO_APP)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsOpenLinkYellowskin() const {
    if (IsClientSupports(OPEN_LINK_YELLOWSKIN)) {
        return true;
    } else if (IsClientUnsupports(OPEN_LINK_YELLOWSKIN)) {
        return false;
    }
    // legacy check, shouldn't be extended
    return IsSearchApp();
}

bool TClientFeatures::SupportsOpenVideoEditor() const {
    return IsClientSupports(OPEN_VIDEO_EDITOR);
}

bool TClientFeatures::SupportsPublicAvailability() const {
    if (IsClientSupports(PUBLICLY_AVAILABLE)) {
        return true;
    } else if (IsClientUnsupports(PUBLICLY_AVAILABLE)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsNoReliableSpeakers() const {
    if (IsClientSupports(NO_RELIABLE_SPEAKERS)) {
        return true;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsNoBluetooth() const {
    if (IsClientSupports(NO_BLUETOOTH)) {
        return true;
    }
    // legacy check, shouldn't be extended
    return !IsSmartSpeaker();
}

bool TClientFeatures::SupportsNoMicrophone() const {
    if (IsClientSupports(NO_MICROPHONE)) {
        return true;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsHandleAndroidAppIntent() const {
    return IsClientSupports(HANDLE_ANDROID_APP_INTENT);
}

bool TClientFeatures::SupportsTvOpenStore() const {
    return IsClientSupports(TV_OPEN_STORE);
}

bool TClientFeatures::SupportsDarkThemeSetting() const {
    return IsClientSupports(DARK_THEME_SETTING);
}

bool TClientFeatures::SupportsReadSites() const {
    return IsClientSupports(READ_SITES);
}

bool TClientFeatures::SupportsVideoCodecAVC() const {
    return IsClientSupports(VIDEO_CODEC_AVC);
}

bool TClientFeatures::SupportsVideoCodecHEVC() const {
    return IsClientSupports(VIDEO_CODEC_HEVC);
}

bool TClientFeatures::SupportsVideoCodecVP9() const {
    return IsClientSupports(VIDEO_CODEC_VP9);
}

bool TClientFeatures::SupportsAudioCodecAAC() const {
    return IsClientSupports(AUDIO_CODEC_AAC);
}

bool TClientFeatures::SupportsAudioCodecAC3() const {
    return IsClientSupports(AUDIO_CODEC_AC3);
}

bool TClientFeatures::SupportsAudioCodecEAC3() const {
    return IsClientSupports(AUDIO_CODEC_EAC3);
}

bool TClientFeatures::SupportsAudioCodecVORBIS() const {
    return IsClientSupports(AUDIO_CODEC_VORBIS);
}

bool TClientFeatures::SupportsAudioCodecOPUS() const {
    return IsClientSupports(AUDIO_CODEC_OPUS);
}

bool TClientFeatures::SupportsDynamicRangeSDR() const {
    return IsClientSupports(DYNAMIC_RANGE_SDR);
}

bool TClientFeatures::SupportsDynamicRangeHDR10() const {
    return IsClientSupports(DYNAMIC_RANGE_HDR10);
}

bool TClientFeatures::SupportsDynamicRangeHDR10Plus() const {
    return IsClientSupports(DYNAMIC_RANGE_HDR10PLUS);
}

bool TClientFeatures::SupportsDynamicRangeDV() const {
    return IsClientSupports(DYNAMIC_RANGE_DV);
}

bool TClientFeatures::SupportsDynamicRangeHLG() const {
    return IsClientSupports(DYNAMIC_RANGE_HLG);
}

bool TClientFeatures::SupportsAudioBitrate192Kbps() const {
    return IsClientSupports(AUDIO_BITRATE_192);
}

bool TClientFeatures::SupportsAudioBitrate320Kbps() const {
    return IsClientSupports(AUDIO_BITRATE_320);
}

bool TClientFeatures::SupportsOpenQuasarScreen() const {
    if (IsClientSupports(QUASAR_SCREEN)) {
        return true;
    } else if (IsClientUnsupports(QUASAR_SCREEN)) {
        return false;
    }
    // legacy check, shouldn't be extended
    return IsSearchApp();
}

bool TClientFeatures::SupportsOpenWhocalls() const {
    if (IsClientSupports(WHOCALLS)) {
        return true;
    } else if (IsClientUnsupports(WHOCALLS)) {
        return false;
    }
    // legacy check, shouldn't be extended
    return IsSearchApp()
        && (IsAndroidAppOfVersionOrNewer(7, 4) || IsIOSAppOfVersionOrNewer(4, 9));
}

bool TClientFeatures::SupportsOpenWhocallsBlocking() const {
    return IsClientSupports(WHOCALLS_CALL_BLOCKING);
}

bool TClientFeatures::SupportsOpenWhocallsMessageFiltering() const {
    return IsClientSupports(WHOCALLS_MESSAGE_FILTERING);
}

bool TClientFeatures::SupportsOpenPasswordManager() const {
    if (IsClientSupports(OPEN_PASSWORD_MANAGER)) {
        return true;
    }
    if (IsClientUnsupports(OPEN_PASSWORD_MANAGER)) {
        return false;
    }
    return (IsSearchApp() || IsYaBrowserMobile()) && (IsAndroidAppOfVersionOrNewer(20, 12, 5) || IsIOSAppOfVersionOrNewer(21, 5, 1));
}

bool TClientFeatures::SupportsGoHomeDirective() const {
    if (IsClientSupports(GO_HOME)) {
        return true;
    } else if (IsClientUnsupports(GO_HOME)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsAbsoluteVolumeChange() const {
    if (IsClientSupports(ABSOLUTE_VOLUME_CHANGE)) {
        return true;
    } else if (IsClientUnsupports(ABSOLUTE_VOLUME_CHANGE)) {
        return false;
    }
    // default
    return true;
}

bool TClientFeatures::SupportsRelativeVolumeChange() const {
    if (IsClientSupports(RELATIVE_VOLUME_CHANGE)) {
        return true;
    } else if (IsClientUnsupports(RELATIVE_VOLUME_CHANGE)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsDoNotDisturbDirective() const {
    if (IsClientSupports(DO_NOT_DISTURB)) {
        return true;
    } else if (IsClientUnsupports(DO_NOT_DISTURB)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsWhisper() const {
    if (IsClientSupports(WHISPER)) {
        return true;
    } else if (IsClientUnsupports(WHISPER)) {
        return false;
    }
    // See https://st.yandex-team.ru/MEGAMIND-3226#61a68da4dab7af733c9018b4
    return IsYaBrowser() || IsYaLauncher() || IsSearchApp() || IsSmartSpeaker();
}

bool TClientFeatures::SupportsPhoneAssistant() const {
    return IsClientSupports(PHONE_ASSISTANT);
}

bool TClientFeatures::SupportsOutgoingDeviceCalls() const {
    if (IsClientSupports(OUTGOING_DEVICE_CALLS) || IsClientSupports(OUTGOING_MESSENGER_CALLS_FEATURE)) {
        return true;
    } else if (IsClientUnsupports(OUTGOING_DEVICE_CALLS) || IsClientUnsupports(OUTGOING_MESSENGER_CALLS_FEATURE)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsPlayerContinueDirective() const {
    if (IsClientSupports(PLAYER_CONTINUE_DIRECTIVE)) {
        return true;
    } else if (IsClientUnsupports(PLAYER_CONTINUE_DIRECTIVE)) {
        return false;
    }

    // default
    return SupportsAnyPlayer();
}

bool TClientFeatures::SupportsPlayerDislikeDirective() const {
    if (IsClientSupports(PLAYER_DISLIKE_DIRECTIVE)) {
        return true;
    } else if (IsClientUnsupports(PLAYER_DISLIKE_DIRECTIVE)) {
        return false;
    }

    // default
    return SupportsAnyPlayer();
}

bool TClientFeatures::SupportsPlayerLikeDirective() const {
    if (IsClientSupports(PLAYER_LIKE_DIRECTIVE)) {
        return true;
    } else if (IsClientUnsupports(PLAYER_LIKE_DIRECTIVE)) {
        return false;
    }

    // default
    return SupportsAnyPlayer();
}

bool TClientFeatures::SupportsPlayerNextTrackDirective() const {
    if (IsClientSupports(PLAYER_NEXT_TRACK_DIRECTIVE)) {
        return true;
    } else if (IsClientUnsupports(PLAYER_NEXT_TRACK_DIRECTIVE)) {
        return false;
    }

    // default
    return SupportsAnyPlayer();
}

bool TClientFeatures::SupportsPlayerPauseDirective() const {
    if (IsClientSupports(PLAYER_PAUSE_DIRECTIVE)) {
        return true;
    } else if (IsClientUnsupports(PLAYER_PAUSE_DIRECTIVE)) {
        return false;
    }

    // default
    return true;
}

bool TClientFeatures::SupportsPlayerPreviousTrackDirective() const {
    if (IsClientSupports(PLAYER_PREVIOUS_TRACK_DIRECTIVE)) {
        return true;
    } else if (IsClientUnsupports(PLAYER_PREVIOUS_TRACK_DIRECTIVE)) {
        return false;
    }

    // default
    return SupportsAnyPlayer();
}

bool TClientFeatures::SupportsPlayerRewindDirective() const {
    if (IsClientSupports(PLAYER_REWIND_DIRECTIVE)) {
        return true;
    } else if (IsClientUnsupports(PLAYER_REWIND_DIRECTIVE)) {
        return false;
    }

    // default
    return SupportsAnyPlayer();
}

bool TClientFeatures::SupportsShowPromo() const {
    return IsClientSupports(SHOW_PROMO);
}

bool TClientFeatures::SupportsOpenLinkOutgoingDeviceCalls() const {
    if (IsClientSupports(OPEN_LINK_OUTGOING_DEVICE_CALLS)) {
        return true;
    } else if (IsClientUnsupports(OPEN_LINK_OUTGOING_DEVICE_CALLS)) {
        return false;
    }
    // default
    return false;
}

bool TClientFeatures::SupportsOutgoingOperatorCalls() const {
    return IsClientSupports(OUTGOING_OPERATOR_CALLS);
}

bool TClientFeatures::SupportsRouteManagerCapability() const {
    return IsClientSupports(ROUTE_MANAGER_CONTINUE) && IsClientSupports(ROUTE_MANAGER_SHOW) &&
           IsClientSupports(ROUTE_MANAGER_START) && IsClientSupports(ROUTE_MANAGER_STOP);
}

} // namespace NAlice
