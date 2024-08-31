#include "devices_settings.h"

#include <alice/hollywood/library/scenarios/settings/common/names.h>

#include <alice/hollywood/library/response/push.h>

namespace NAlice::NHollywood::NSettings {

namespace {

const TString YELLOWSKIN_TANDEM_SETTINGS_URL = "yellowskin://?url=https%3A%2F%2Fyandex.ru%2Fquasar%2Fexternal%2Fcreate-tandem";
const TString IOS_TANDEM_SETTINGS_URL = "yandexiot://?url=yandex.ru%2Fiot%2Fexternal%2Fcreate-tandem";
const TString ANDROID_TANDEM_SETTINGS_URL = "yandexiot://?uri=yandex.ru%2Fiot%2Fexternal%2Fcreate-tandem";
const TString IOS_ADD_SMART_SPEAKER_URL = "yandexiot://?url=yandex.ru%2Fiot%2Fexternal%2Fadd-speaker";
const TString ANDROID_ADD_SMART_SPEAKER_URL = "yandexiot://?uri=yandex.ru%2Fiot%2Fexternal%2Fadd-speaker";
const TString OPEN_SETTINGS_QUASAR_URL = "opensettings://?screen=quasar";
const TString TANDEOM_SETUP_ANDROID_INTENT_ACTION = "com.yandex.tv.action.TANDEM_SETUP";
const TString OPEN_URI_DIRECTIVE_NAME = "open_uri";
const TString ADD_SMART_SPEAKER_SCREEN_DIRECTIVE_NAME = "add_smart_speaker_screen";

constexpr TStringBuf TANDEM_SETTING_TEMPLATE = "tandem_setting";
constexpr TStringBuf SEND_PUSH_PHRASE = "send_push";
constexpr TStringBuf RENDER_RESULT_PHRASE = "render_result";
constexpr TStringBuf OPEN_DEVICES_SETTINGS_TEMPLATE = "open_devices_settings";
constexpr TStringBuf UNSUPPORTED_SURFACE_PHRASE = "unsupported_surface";

constexpr TStringBuf FLAG_USE_TANDEM_IN_TV = "use_tandem_in_tv";


TResponseBodyBuilder::TSuggest RenderSuggestWithOpenUri(const TString& uri, const TString& subName = OPEN_URI_DIRECTIVE_NAME) {
    TDirective directive;
    auto& openUriDirective = *directive.MutableOpenUriDirective();
    openUriDirective.SetName(subName);
    openUriDirective.SetUri(uri);
    TResponseBodyBuilder::TSuggest suggest;
    suggest.ButtonForText = "Открыть";
    suggest.Directives.push_back(directive);
    suggest.AutoDirective = std::move(directive);
    return suggest;
}

TDirective RenderTandemSetupDirective() {
    TDirective directive;
    auto& sendAndroidIntentDirective = *directive.MutableSendAndroidAppIntentDirective();
    sendAndroidIntentDirective.MutableFlags()->SetFlagActivityNewTask(true);
    sendAndroidIntentDirective.SetAction(TANDEOM_SETUP_ANDROID_INTENT_ACTION);
    return directive;
}

}

// TDevicesSettings ---------------------------------------------------------------
bool TDevicesSettings::HandleFrame(const TOpenTandemSettingSemanticFrame&) {
    // TODO(@yagafarov): Erase app_id check after s_f fully released
    if (Ctx.Request.Interfaces().GetSupportsTandemSetup() || Ctx.Request.ClientInfo().IsTvDevice()) {
        RenderTandemSettingScreen(ESettingObject::Tandem);
        return true;
    } else if (Ctx.Request.Interfaces().GetCanOpenLinkYellowskin()) {
        RenderTandemSettingStory(YELLOWSKIN_TANDEM_SETTINGS_URL);
        return true;
    }

    if (Ctx.Request.ClientInfo().IsIotApp()) {
        if (Ctx.Request.ClientInfo().IsIOS()) {
            RenderTandemSettingStory(IOS_TANDEM_SETTINGS_URL);
            return true;
        } else if (Ctx.Request.ClientInfo().IsAndroid()) {
            RenderTandemSettingStory(ANDROID_TANDEM_SETTINGS_URL);
            return true;
        }
    }
    if (!GetUid(Ctx.Request)) {
        RenderNoAuth();
    } else {
        RenderTandemPushDirective();
    }
    
    return true;
}

bool TDevicesSettings::HandleFrame(const TOpenSmartSpeakerSettingSemanticFrame&) {
    if (
        Ctx.Request.Interfaces().GetSupportsTandemSetup() && (!Ctx.Request.ClientInfo().IsTvDevice() || 
        Ctx.Request.ClientInfo().IsYaModule2()) || 
        Ctx.Request.ClientInfo().IsTvDevice() && Ctx.Request.HasExpFlag(FLAG_USE_TANDEM_IN_TV) // TVANDROID-6798
    ) {
        RenderTandemSettingScreen(ESettingObject::SmartSpeaker);
        return true;
    }
    if (Ctx.Request.ClientInfo().IsIotApp()) {
        if (Ctx.Request.ClientInfo().IsIOS()) {
            RenderAddSmartSpeakerScreen(IOS_ADD_SMART_SPEAKER_URL);
            return true;
        } else if (Ctx.Request.ClientInfo().IsAndroid()) {
            RenderAddSmartSpeakerScreen(ANDROID_ADD_SMART_SPEAKER_URL);
            return true;
        }
    }
    if (Ctx.Request.Interfaces().GetCanOpenQuasarScreen()) {
        RenderAddSmartSpeakerScreen(OPEN_SETTINGS_QUASAR_URL);
        return true;
    }
    RenderCantOpenDevicesSettingsScreen();
    return true;
}

void TDevicesSettings::RenderCantOpenDevicesSettingsScreen() const {
    Ctx.BodyBuilder.AddRenderedTextAndVoice(OPEN_DEVICES_SETTINGS_TEMPLATE, UNSUPPORTED_SURFACE_PHRASE, Ctx.NlgData);
}

void TDevicesSettings::RenderTandemSettingStory(const TString& url) const {
    Ctx.BodyBuilder.AddRenderedSuggest(RenderSuggestWithOpenUri(url));
    RenderTandemSettingTextAndVoice(ESettingObject::Tandem);
}

void TDevicesSettings::RenderTandemSettingScreen(const ESettingObject settingObject) const {
    Ctx.BodyBuilder.AddDirective(RenderTandemSetupDirective());
    RenderTandemSettingTextAndVoice(settingObject);
}

void TDevicesSettings::RenderTandemSettingTextAndVoice(const ESettingObject settingObject) const {
    TNlgData nlgData{Ctx.NlgData};
    nlgData.Context["setting_object"] = ToString(settingObject);
    Ctx.BodyBuilder.AddRenderedTextAndVoice(TANDEM_SETTING_TEMPLATE, RENDER_RESULT_PHRASE, nlgData);
}

void TDevicesSettings::RenderAddSmartSpeakerScreen(const TString& url) const {
    Ctx.BodyBuilder.AddRenderedSuggest(
        RenderSuggestWithOpenUri(url, /* subName= */ ADD_SMART_SPEAKER_SCREEN_DIRECTIVE_NAME));
    Ctx.BodyBuilder.AddRenderedTextAndVoice(OPEN_DEVICES_SETTINGS_TEMPLATE, RENDER_RESULT_PHRASE, Ctx.NlgData);
}

void TDevicesSettings::RenderTandemPushDirective() const {
    TPushDirectiveBuilder builder{/* title= */ "Настройка Тандема", /* text= */ "Открыть страницу настройки Тандема",
                                  YELLOWSKIN_TANDEM_SETTINGS_URL, /* tag= */ "open_site_or_app"};
    builder.SetThrottlePolicy("unlimited_policy")
        .SetTtlSeconds(900)
        .SetAnalyticsAction("send_tandem_settings", "send tandem settings",
                            "Отправляется ссылка на экран настройки тандема");
    builder.BuildTo(Ctx.BodyBuilder);

    Ctx.BodyBuilder.AddRenderedTextAndVoice(TANDEM_SETTING_TEMPLATE, SEND_PUSH_PHRASE, Ctx.NlgData);
    Ctx.BodyBuilder.SetShouldListen(true);
}

void TDevicesSettings::RenderNoAuth() const {
    Ctx.BodyBuilder.AddRenderedTextAndVoice(SCENARIO_NAME, RENDER_UNAUTHORIZED, Ctx.NlgData);
    Ctx.BodyBuilder.SetShouldListen(true);
}

} // namespace NAlice::NHollywood
