#include "push_message.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/video/mordovia_webview_settings.h>
#include <alice/bass/forms/video/utils.h>
#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/mordovia_webview_defs.h>

#include <library/cpp/string_utils/quote/quote.h>

using namespace NAlice::NVideoCommon;

namespace {
inline constexpr TStringBuf TITLE_NAME_MASK = "%name%";
// Billing landing
// Host
inline constexpr TStringBuf DEFAULT_VIDEO_BILLING_LANDING_HOST = "https://yandex.ru";
inline constexpr TStringBuf FLAG_VIDEO_BILLING_LANDING_HOST = "video_billing_landing_host";
// Path
inline constexpr TStringBuf DEFAULT_VIDEO_BILLING_LANDING_PATH = "/video/quasar/billingLanding/";
inline constexpr TStringBuf FLAG_VIDEO_BILLING_LANDING_PATH = "video_billing_landing_path";
// CGI params for billing landing webview screens
inline constexpr TStringBuf CGI_PARAM_FROM = "from";
// Distinct "From" param values
inline constexpr TStringBuf FROM_PERSONAL_CARD = "personal_card";
inline constexpr TStringBuf FROM_PUSH_MESSAGE = "push_message";
inline constexpr TStringBuf FROM_QR_CODE = "qr_code";

// Push message
// Throttle policy
inline constexpr TStringBuf DEFAULT_VIDEO_BUY_PUSH_POLICY = "bass-default-push";
inline constexpr TStringBuf FLAG_VIDEO_BUY_PUSH_POLICY = "video_buy_push_policy";
// Title
inline constexpr TStringBuf DEFAULT_VIDEO_BUY_PUSH_TITLE = TITLE_NAME_MASK;
inline constexpr TStringBuf FLAG_VIDEO_BUY_PUSH_TITLE = "video_buy_push_title";
// Text
inline constexpr TStringBuf DEFAULT_VIDEO_BUY_PUSH_TEXT = "Нажмите для продолжения покупки";
inline constexpr TStringBuf FLAG_VIDEO_BUY_PUSH_TEXT = "video_buy_push_text";

// Personal card
// TTL
inline constexpr TStringBuf DEFAULT_VIDEO_BUY_PERSONAL_CARD_TTL = "600"; // in seconds
inline constexpr TStringBuf FLAG_VIDEO_BUY_PERSONAL_CARD_TTL = "video_buy_personal_card_ttl";
// Title
inline constexpr TStringBuf DEFAULT_VIDEO_BUY_PERSONAL_CARD_TITLE = "Купить и смотреть";
inline constexpr TStringBuf FLAG_VIDEO_BUY_PERSONAL_CARD_TITLE = "video_buy_personal_card_title";
// Text
inline constexpr TStringBuf DEFAULT_VIDEO_BUY_PERSONAL_CARD_TEXT = TITLE_NAME_MASK;
inline constexpr TStringBuf FLAG_VIDEO_BUY_PERSONAL_CARD_TEXT = "video_buy_personal_card_text";

// Tag
inline constexpr TStringBuf DEFAULT_VIDEO_BUY_TAG = "alice_video_buy";
inline constexpr TStringBuf FLAG_VIDEO_BUY_TAG = "video_buy_tag";

// Additional params in PushMessageParams
inline constexpr TStringBuf PARAM_PERSONAL_CARD_IMAGE_URL = "personal_card_image_url";
inline constexpr TStringBuf PARAM_PERSONAL_CARD_TEXT = "personal_card_text";
inline constexpr TStringBuf PARAM_PERSONAL_CARD_TITLE = "personal_card_title";
inline constexpr TStringBuf PARAM_PERSONAL_CARD_URL = "personal_card_url";
} // namespace

#if defined(FLAG_DEFAULT)
#error FLAG_DEFAULT should not be defined at this point
#endif
#define FLAG_DEFAULT(setting) FLAG_VIDEO_##setting, DEFAULT_VIDEO_##setting

namespace NBASS::NVideo {

namespace {

TString GenerateBillingLandingUrl(const TContext& ctx, TVideoItemConstScheme item, const TStringBuf from, const TCgiParameters& addParams = TCgiParameters()) {
    TString host = GetStringSettingsFromExp(ctx, FLAG_DEFAULT(BILLING_LANDING_HOST));
    TString path = GetStringSettingsFromExp(ctx, FLAG_DEFAULT(BILLING_LANDING_PATH));

    TCgiParameters buyParams;
    buyParams.InsertUnescaped(TStringBuf("deviceid"), ctx.GetDeviceId());
    buyParams.InsertUnescaped(WEBVIEW_PARAM_UUIDS, item->ProviderItemId());
    buyParams.InsertUnescaped(CGI_PARAM_FROM, from);
    AddTestidsToCgiParams(ctx, buyParams);
    for (const auto& it : addParams) {
        buyParams.InsertUnescaped(it.first, it.second);
    }

    return BuildWebViewVideoUrl(host, path, buyParams);
}

TString MakeQrUrl(const TString& url) {
    return "https://disk.yandex.net/qr/?clean=1&text=" + CGIEscapeRet(url);
}

TString GetStringSettingWithTitleName(TContext& ctx, const TStringBuf flagName, const TStringBuf defaultValue, const TString& titleName) {
    TString setting = GetStringSettingsFromExp(ctx, flagName, defaultValue);
    SubstGlobal(setting, TITLE_NAME_MASK, titleName);
    return setting;
}

} // namespace

void AddPushMessageResponseDirective(TContext& ctx, const PushMessageParams& pushMessageParams) {
    NSc::TValue payload;
    payload["title"].SetString(pushMessageParams.Title);
    payload["text"].SetString(pushMessageParams.Text);
    payload["url"].SetString(pushMessageParams.Url);
    payload["tag"].SetString(pushMessageParams.Tag);
    payload["id"].SetString(pushMessageParams.Id);
    payload["throttle"].SetString(pushMessageParams.ThrottlePolicy);

    TString ttl = GetStringSettingsFromExp(ctx, FLAG_DEFAULT(BUY_PERSONAL_CARD_TTL));
    payload["ttl"].SetIntNumber(FromStringWithDefault<ui64>(ttl, FromString<ui64>(DEFAULT_VIDEO_BUY_PERSONAL_CARD_TTL)));
    payload["remove_existing_cards"].SetBool(false);

    auto action = payload["actions"].SetArray();
    auto unrelated = action.Push();
    unrelated["id"].SetString("close");
    unrelated["title"].SetString("Закрыть");

    //payload["min_price"].SetIntNumber(); // Now we haven't such param
    for (const auto& param : pushMessageParams.AdditionalParams) {
        payload[param.first].SetString(param.second);
    }
    ctx.AddCommand<TSendPushDirective>(COMMAND_SEND_PUSH, payload);
}

void AddBuyPushMessageResponseDirective(TContext& ctx, TVideoItemConstScheme item, const TCgiParameters& addParams) {
    TString titleName = ToString(item->Name());
    PushMessageParams buyPushParams;
    buyPushParams.Id = GetStringSettingsFromExp(ctx, FLAG_DEFAULT(BUY_TAG));
    buyPushParams.Tag = GetStringSettingsFromExp(ctx, FLAG_DEFAULT(BUY_TAG));
    buyPushParams.Text = GetStringSettingWithTitleName(ctx, FLAG_DEFAULT(BUY_PUSH_TEXT), titleName);
    buyPushParams.ThrottlePolicy = GetStringSettingsFromExp(ctx, FLAG_DEFAULT(BUY_PUSH_POLICY));
    buyPushParams.Title = GetStringSettingWithTitleName(ctx, FLAG_DEFAULT(BUY_PUSH_TITLE), titleName);
    buyPushParams.Url = GenerateBillingLandingUrl(ctx, item, FROM_PUSH_MESSAGE, addParams);

    TStringBuf imageUrl;
    if (item.HasPoster() && item->Poster().HasBaseUrl()) {
        imageUrl = item->Poster().BaseUrl();
    }
    if (imageUrl.Empty()) {
        imageUrl = item.ThumbnailUrl16X9();
    }
    buyPushParams.AdditionalParams[PARAM_PERSONAL_CARD_IMAGE_URL] = imageUrl;
    buyPushParams.AdditionalParams[PARAM_PERSONAL_CARD_TEXT] = GetStringSettingWithTitleName(ctx, FLAG_DEFAULT(BUY_PERSONAL_CARD_TEXT), titleName);
    buyPushParams.AdditionalParams[PARAM_PERSONAL_CARD_TITLE] = GetStringSettingWithTitleName(ctx, FLAG_DEFAULT(BUY_PERSONAL_CARD_TITLE), titleName);
    buyPushParams.AdditionalParams[PARAM_PERSONAL_CARD_URL] = GenerateBillingLandingUrl(ctx, item, FROM_PERSONAL_CARD, addParams);

    AddPushMessageResponseDirective(ctx, buyPushParams);
}

#undef FLAG_DEFAULT

void AddShowBuyPushScreenResponseDirective(TContext& ctx, TVideoItemConstScheme item, const TCgiParameters& addParams) {
    TCgiParameters params;
    params.InsertUnescaped(WEBVIEW_PARAM_LANDING_URL, CGIEscapeRet(MakeQrUrl(GenerateBillingLandingUrl(ctx, item, FROM_QR_CODE, addParams))));
    AddShowWebviewVideoEntityResponse(item, ctx, false, params);
}

void AddBuyPushScreenAndPushMessageDirectives(TContext& ctx, TVideoItemConstScheme item, const TCgiParameters& addParams) {
    AddBuyPushMessageResponseDirective(ctx, item, addParams);
    AddShowBuyPushScreenResponseDirective(ctx, item, addParams);
}

}
