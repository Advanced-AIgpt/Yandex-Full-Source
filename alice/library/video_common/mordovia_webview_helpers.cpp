#include "mordovia_webview_helpers.h"
#include "mordovia_webview_defs.h"

#include <alice/library/proto/proto_struct.h>

namespace NAlice::NVideoCommon {

namespace {

void CollectVideoItemInfo(const NSc::TValue& webViewItem, NSc::TValue& resultItem) {
    resultItem["provider_info"] = webViewItem.TrySelect("provider_info");
    resultItem["provider_name"].SetString(webViewItem.TrySelect("provider_name").GetString());
    resultItem["provider_item_id"].SetString(webViewItem.TrySelect("provider_item_id").GetString());
    resultItem["type"].SetString(webViewItem.TrySelect("provider_info/0/type").GetString());
    if (resultItem["type"].GetString().empty()) {
        // for tv channels type is in root
        resultItem["type"].SetString(webViewItem.TrySelect("type").GetString());
    }
    // All boolean fields in TVideoItem proto are marked as uint32 in order to keep scheme compatibility
    resultItem["available"].SetIntNumber(webViewItem.TrySelect("provider_info/0/available").GetBool());

    resultItem["entref"].SetString(webViewItem.TrySelect("entref").GetString());

    resultItem["name"].SetString(webViewItem.TrySelect("title").GetString());
    resultItem["description"].SetString(webViewItem.TrySelect("description").GetString());

    resultItem["season"].SetIntNumber(webViewItem.TrySelect("provider_info/0/season").GetIntNumber());
    resultItem["episode"].SetIntNumber(webViewItem.TrySelect("provider_info/0/episode").GetIntNumber());

    resultItem["seasons_count"].SetIntNumber(webViewItem.TrySelect("seasons_count").GetIntNumber());
    resultItem["episodes_count"].SetIntNumber(webViewItem.TrySelect("episodes_count").GetIntNumber());
    const auto& mainTrailerUuid = webViewItem.TrySelect("main_trailer_uuid").GetString();
    if (!mainTrailerUuid.empty()) {
        resultItem["main_trailer_uuid"].SetString(mainTrailerUuid);
    }
    const auto& related = webViewItem.TrySelect("related");
    if (!related.IsNull()) {
        resultItem["related"] = related;
    }
    // Collecting video item meta info from 'metaforlog'
    resultItem["duration"].SetIntNumber(webViewItem.TrySelect("metaforlog/duration").GetIntNumber());
    resultItem["genre"].SetString(webViewItem.TrySelect("metaforlog/genres").GetString());
    const auto& restrictionAge = webViewItem.TrySelect("metaforlog/restriction_age");
    if (!restrictionAge.IsNull()) {
        resultItem["min_age"].SetIntNumber(restrictionAge.GetIntNumber());
        resultItem["age_limit"].SetString(ToString(restrictionAge.GetIntNumber()));
    }
    resultItem["rating"].SetNumber(webViewItem.TrySelect("metaforlog/rating_kp").GetNumber());
    resultItem["release_year"].SetIntNumber(webViewItem.TrySelect("metaforlog/release_year").GetIntNumber());
    resultItem["view_count"].SetIntNumber(webViewItem.TrySelect("metaforlog/views_count").GetIntNumber());
}

} // namespace

TStringBuf GetWebViewScreenName(const NSc::TValue& viewState) {
    return viewState.TrySelect("currentScreen").GetString();
}
TStringBuf GetWebViewScreenName(const google::protobuf::Struct& viewState) {
    TProtoStructParser parser;
    return parser.GetValueString(viewState, "currentScreen", /* defaultValue= */ "");
}

const NSc::TArray& GetWebViewGalleryItems(const NSc::TValue& viewState) {
    return viewState.TrySelect("sections/0/items").GetArray();
}

const google::protobuf::ListValue& GetWebViewGalleryItems(const google::protobuf::Struct& viewState) {
    TProtoStructParser parser;
    return parser.GetArray(viewState, "sections.0.items");
}

const NSc::TValue& GetWebViewCurrentVideoItem(const NSc::TValue& viewState) {
    return viewState.TrySelect("sections/0/current_item");
}

const google::protobuf::Struct& GetWebViewCurrentVideoItem(const google::protobuf::Struct& viewState) {
    TProtoStructParser parser;
    return parser.GetKey(viewState, "sections.0.current_item");
}

const NSc::TValue& GetWebViewCurrentTvShowItem(const NSc::TValue& viewState) {
    return viewState.TrySelect("sections/0/current_tv_show_item");
}

const google::protobuf::Struct& GetWebViewCurrentTvShowItem(const google::protobuf::Struct& viewState) {
    TProtoStructParser parser;
    return parser.GetKey(viewState, "sections.0.current_tv_show_item");
}

const TMap<size_t, NSc::TValue> GetVisibleGalleryItems(const NSc::TValue& viewState) {
    TMap<size_t, NSc::TValue> result;
    for (const auto& webViewItem : GetWebViewGalleryItems(viewState)) {
        if (!webViewItem.TrySelect("active").GetBool()) {
            continue;
        }

        NSc::TValue item;
        CollectVideoItemInfo(webViewItem, item);

        result[webViewItem.TrySelect("number").GetIntNumber()] = item;
    }

    return result;
}

NSc::TValue GetRawVideoGallery(const NSc::TValue& viewState) {
    NSc::TValue gallery;
    NSc::TValue& items = gallery["items"];
    for (const auto& webViewItem : GetWebViewGalleryItems(viewState)) {
        NSc::TValue& item = items.Push();
        CollectVideoItemInfo(webViewItem, item);
    }

    return gallery;
}

NSc::TValue GetRawCurrentVideoItem(const NSc::TValue& viewState) {
    NSc::TValue videoItem;
    CollectVideoItemInfo(GetWebViewCurrentVideoItem(viewState), videoItem);
    return videoItem;
}

NSc::TValue GetRawCurrentTvShowItem(const NSc::TValue& viewState) {
    NSc::TValue videoItem;
    CollectVideoItemInfo(GetWebViewCurrentTvShowItem(viewState), videoItem);
    return videoItem;
}

i64 GetCurrentTvShowSeasonNumber(const NSc::TValue& viewState) {
    return viewState.TrySelect("sections/0/season").GetIntNumber();
}

TStringBuf GetUserSubscriptionType(const NSc::TValue& viewState) {
    return viewState["user_subscription_type"].GetString();
}

TStringBuf GetCurrentViewKey(const TDeviceState& deviceState) {
    TStringBuf currentViewKey = deviceState.GetVideo().GetScreenState().GetViewKey();
    if (currentViewKey.empty()) {
        // "scenario" field in device_state/video/screen_state is deprecated since QUASAR-7413
        currentViewKey = deviceState.GetVideo().GetScreenState().GetScenario();
    }
    return currentViewKey;
}

TStringBuf GetCurrentViewKey(const NSc::TValue& deviceState) {
    TStringBuf currentViewKey = deviceState.TrySelect("video/screen_state/view_key").GetString();
    if (currentViewKey.empty()) {
        // "scenario" field in device_state/video/screen_state is deprecated since QUASAR-7413
        currentViewKey = deviceState.TrySelect("video/screen_state/scenario").GetString();
    }
    return currentViewKey;
}

bool IsWebViewMainViewKey(TStringBuf viewKey) {
    return viewKey == VIDEO_STATION_SPA_MAIN_VIEW_KEY;
}

bool IsOneOfThePossibleOldVideoViewKeys(TStringBuf viewKey) {
    TStringBuf scenarioName = viewKey.NextTok(':');
    return viewKey == "video" &&
        (scenarioName.Empty() || scenarioName == "Video" || scenarioName == "MordoviaVideoSelection");
}

bool IsWebViewVideoViewKey(TStringBuf viewKey) {
    return viewKey == VIDEO_STATION_SPA_VIDEO_VIEW_KEY ||
           IsWebViewMainViewKey(viewKey) ||
           IsOneOfThePossibleOldVideoViewKeys(viewKey);
}

NSc::TValue BuildMordoviaShowPayload(TStringBuf viewKey, const TString& url, const TString& splashDiv, bool doGoBack) {
    NSc::TValue payload;
    payload["url"].SetString(url);
    payload["view_key"].SetString(viewKey);
    payload["splash_div"].SetString(splashDiv);
    payload["callback_name"].SetString(viewKey);
    payload["go_back"].SetBool(doGoBack); // see QUASAR-7802
    return payload;
}

NJson::TJsonValue BuildMordoviaShowJsonPayload(TStringBuf viewKey, const TString& url, const TString& splashDiv, bool doGoBack) {
    return BuildMordoviaShowPayload(viewKey, url, splashDiv, doGoBack).ToJsonValue();
}

NSc::TValue BuildChangePathCommandPayload(TStringBuf viewKey, const TString& path, bool clearNavigationHistory) {
    NSc::TValue payload;
    payload["command"].SetString("change_path");
    payload["view_key"].SetString(viewKey);
    payload["meta"]["path"].SetString(path);
    payload["meta"]["clear_history"].SetBool(clearNavigationHistory);

    return payload;
}

NJson::TJsonValue BuildChangePathCommandJsonPayload(TStringBuf viewKey, const TString& path, bool clearNavigationHistory) {
    return BuildChangePathCommandPayload(viewKey, path, clearNavigationHistory).ToJsonValue();
}

TStringBuf GetTestidsFromMegamindCookies(const ::google::protobuf::Struct& megamindCookies) {
    TString buffer;
    google::protobuf::util::MessageToJsonString(megamindCookies, &buffer);
    return GetTestidsFromMegamindCookies(TStringBuf{buffer});
}

TStringBuf GetTestidsFromMegamindCookies(const TStringBuf megamindCookies) {
    TStringBuf testids;
    NJson::TJsonValue cookies;
    if (NJson::ReadJsonTree(megamindCookies, &cookies)) {
        testids = JoinStrings(cookies["uaas_tests"].GetArray().begin(), cookies["uaas_tests"].GetArray().end(), "_");
    }
    return testids;
}

} // namespace NAlice::NVideoCommon
