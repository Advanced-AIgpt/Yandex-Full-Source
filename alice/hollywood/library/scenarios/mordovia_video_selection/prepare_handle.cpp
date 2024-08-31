#include "prepare_handle.h"

#include "mordovia_tabs.h"
#include "util.h"

#include <alice/hollywood/library/environment_state/environment_state.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/geo/protos/user_location.pb.h>

#include <alice/library/experiments/flags.h>
#include <alice/library/network/headers.h>
#include <alice/library/proto/proto_struct.h>
#include <alice/library/video_common/device_helpers.h>
#include <alice/library/video_common/mordovia_webview_defs.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>
#include <alice/library/video_common/video_helper.h>
#include <alice/library/video_common/frontend_vh_helpers/frontend_vh_requests.h>
#include <alice/library/video_common/hollywood_helpers/util.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>


using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NMordovia {

namespace {

constexpr TStringBuf DEFAULT_ETHER_PATH = "/video/quasar/home/";

bool IsWebviewPromoUrl(const TStringBuf url) {
    return url.find(MORDOVIA_WEBVIEW_PATTERN) != TStringBuf::npos;
}

void ShowWebviewPromoUrl(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, const TStringBuf url) {
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    TNlgData nlgData(ctx.Ctx.Logger(), request);
    nlgData.AddAttention(NAlice::NVideoCommon::ATTENTION_SHOW_PROMO_WEBVIEW);
    if (!IsSilentResponse(request)) {
        bodyBuilder.AddRenderedTextWithButtonsAndVoice("mordovia", "render_result", /* buttons = */ {}, nlgData);
    }
    TString webviewUrl = PrepareWebviewUrlForPromo(url, request);
    TString splashDiv = ToString(NAlice::NVideoCommon::DEFAULT_PROMO_NY_SPLASH);
    if (const auto expSplash = request.ExpFlag(NAlice::NVideoCommon::FLAG_WEBVIEW_VIDEO_PROMO_SPLASH); expSplash.Defined()) {
        splashDiv = *expSplash;
    }
    AddWebViewCommand(request, bodyBuilder, webviewUrl, splashDiv, /* oldViewKey */ TStringBuf{"video"});
    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

TMaybe<int> GetVideoIndexFromFrame(const TPtrWrapper<NAlice::TSemanticFrame>& semanticFrame, TRTLogger& logger) {
    auto frame = TFrame::FromProto(*semanticFrame);

    if (const TPtrWrapper<TSlot> slot = frame.FindSlot(NAlice::NVideoCommon::SLOT_VIDEO_INDEX)) {
        LOG_INFO(logger) << "Got index slot value: " << slot->Value.AsString();
        return slot->Value.As<ui32>();
    }

    if (const TPtrWrapper<TSlot> slot = frame.FindSlot(NAlice::NVideoCommon::SLOT_VIDEO_ITEM);
        slot && slot->Type == NAlice::NVideoCommon::SLOT_VIDEO_GALLERY_TYPE
    ) {
        LOG_INFO(logger) << "Got item slot value: " << slot->Value.AsString();
        return slot->Value.As<ui32>();
    }

    return Nothing();
}

TMaybe<int> GetVideoIndexFromCallback(const NScenarios::TCallbackDirective* callback) {
    NVideoCommon::TMordoviaJsCallbackPayload jsCallbackPayload(callback->GetPayload());
    if (jsCallbackPayload.Command() == SELECT_VIDEO_CALLBACK ||
        jsCallbackPayload.Command() == SELECT_VIDEO_CALLBACK_DEPRECATED) {
        const auto videoIndex = jsCallbackPayload.Payload().TrySelect(NAlice::NVideoCommon::SLOT_VIDEO_INDEX);
        if (videoIndex.IsIntNumber()) {
            return videoIndex.GetIntNumber();
        }
    }
    return Nothing();
}

TMaybe<int> GetSelectedVideoIndexFromRequest(const TScenarioRunRequestWrapper& request, TRTLogger& logger) {
    if (const auto *callback = request.Input().GetCallback()) {
        return GetVideoIndexFromCallback(callback);
    }

    if (const auto semanticFrame = request.Input().FindSemanticFrame(VIDEO_SELECTION_BY_REMOTE_CONTROL_FRAME)) {
        return GetVideoIndexFromFrame(semanticFrame, logger);
    }

    if (const auto semanticFrame = request.Input().FindSemanticFrame(VIDEO_SELECTION_FRAME)) {
        TMaybe<int> selectedIndex = GetVideoIndexFromFrame(semanticFrame, logger);
        return selectedIndex;
    }

    return Nothing();
}


TMaybe<NSc::TValue> TryGetSelectedVideo(const TScenarioRunRequestWrapper& request, TRTLogger& logger) {
    TMaybe<int> selectedIndex = GetSelectedVideoIndexFromRequest(request, logger);
    if (!selectedIndex.Defined()) {
        return Nothing();
    }

    const auto* viewStateDataSource = request.GetDataSource(NAlice::EDataSourceType::VIDEO_VIEW_STATE);
    if (viewStateDataSource == nullptr) {
        return Nothing();
    }

    const auto& viewState = viewStateDataSource->GetVideoViewState().GetViewState();
    const auto galleryItems = NVideoCommon::GetWebViewGalleryItems(viewState);

    TProtoStructParser parser;

    for (const auto& elem : galleryItems.values()) {
        const auto& webViewItem = elem.struct_value();
        TProtoStructParser parser;
        auto number = parser.GetValueInt(webViewItem, "number");
        if (number.Defined() && *number != *selectedIndex) {
            continue;
        }

        NSc::TValue item;
        auto uuid = parser.GetValueString(webViewItem, "metaforback.uuid");
        if (uuid.Defined()) {
            item["uuid"].SetString(*uuid);
        }
        auto serialId = parser.GetValueString(webViewItem, "metaforback.serial_id");
        if (serialId.Defined()) {
            item["serial_id"].SetString(*serialId);
        }
        auto url = parser.GetValueString(webViewItem, "metaforback.url");
        if (url.Defined()) {
            item["url"].SetString(*url);
        }
        return MakeMaybe(std::move(item));
    }

    return Nothing();
}

bool ShowMordoviaAsHomeScreenSupported(const TScenarioRunRequestWrapper& request) {
    return !request.HasExpFlag(NAlice::NVideoCommon::FLAG_DISABLE_MORDOVIA_GO_HOME) && request.Interfaces().GetIsTvPlugged();
}

TString UrlWithCgiParams(TStringBuf baseUrl, const TScenarioRunRequestWrapper& request) {
    TStringBuilder result;

    TCgiParameters cgiParams = NVideoCommon::GetDefaultWebviewCgiParams(request);
    NVideoCommon::AddIpregParam(cgiParams, request);

    result << baseUrl << (baseUrl.Contains("?") ? "&" : "?") << cgiParams.Print();

    return result;
}

bool TryOpenMordovia(const TScenarioRunRequestWrapper& request, TScenarioHandleContext& ctx) {
    LOG_DEBUG(ctx.Ctx.Logger()) << "Trying to process open mordovia intent..." << Endl;
    if (!request.Input().FindSemanticFrame(GO_HOME_FRAME) &&
        !request.Input().FindSemanticFrame(OPEN_MORDOVIA_FRAME) &&
        !request.Input().FindSemanticFrame(QUASAR_OPEN_HOME_SCREEN_FRAME))
    {
        LOG_DEBUG(ctx.Ctx.Logger()) << "Processing open mordovia intent failed: no semantic frames for open mordovia intent!" << Endl;
        return false;
    }

    if (request.Input().FindSemanticFrame(GO_HOME_FRAME) && !ShowMordoviaAsHomeScreenSupported(request)) {
        LOG_DEBUG(ctx.Ctx.Logger()) << "Processing open mordovia intent failed: mordovia as go home isn't supported!" << Endl;
        return false;
    }

    TString host{NAlice::NVideoCommon::DEFAULT_WEBVIEW_VIDEO_HOST};
    if (const auto expHost = request.ExpFlag(NVideoCommon::FLAG_WEBVIEW_VIDEO_HOST); expHost.Defined()) {
        host = *expHost;
    }
    TString url = host + DEFAULT_ETHER_PATH;

    const auto etherFlag = request.ExpFlag(NAlice::NExperiments::EXP_ETHER);
    if (etherFlag && etherFlag->StartsWith("http")) {
        url = *etherFlag;
    }

    const TString urlWithCGI = UrlWithCgiParams(url, request);

    TString splashDiv = ToString(NAlice::NVideoCommon::DEFAULT_MORDOVIA_MAIN_SCREEN_SPLASH);
    if (const auto expSplash = request.ExpFlag(NAlice::NVideoCommon::FLAG_MORDOVIA_MAIN_SCREEN_SPLASH); expSplash.Defined()) {
        splashDiv = *expSplash;
    }

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    // always return mordovia_show directive when device asks for main page via "quasar.mordovia.home_screen" semantic frame
    if (request.Input().FindSemanticFrame(QUASAR_OPEN_HOME_SCREEN_FRAME)) {
        TStringBuf viewKey = !request.HasExpFlag(NVideoCommon::FLAG_DISABLE_VIDEO_MORDOVIA_SPA) ? NVideoCommon::VIDEO_STATION_SPA_MAIN_VIEW_KEY : TStringBuf{"ether"};
        const NJson::TJsonValue payload = NVideoCommon::BuildMordoviaShowJsonPayload(viewKey,
                                                                                     urlWithCGI,
                                                                                     TString{splashDiv},
                                                                                     /* doGoBack */ true);
        bodyBuilder.AddClientActionDirective(TString(MORDOVIA_SHOW_DIRECTIVE_NAME), payload);
    } else { // user asks for main page via "go home" request
        AddWebViewCommand(request, bodyBuilder, urlWithCGI, splashDiv, /* oldViewKey */ TStringBuf{"ether"}, /* isMainPage */ true);
        // add clear_queue directive to pause music, see ALICE-9726
        bodyBuilder.AddClientActionDirective(TString(CLEAR_QUEUE_DIRECTIVE_NAME), NJson::TJsonValue());
    }

    NVideoCommon::FillAnalyticsInfo(bodyBuilder, ANALYTICS_GO_HOME_INTENT_NAME, ANALYTICS_PRODUCT_SCENARIO_NAME);

    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);

    LOG_DEBUG(ctx.Ctx.Logger()) << "Processing open mordovia intent succeed!" << Endl;
    return true;
}

bool ShowMordoviaTabsSupported(const TScenarioRunRequestWrapper& request) {
    return !request.HasExpFlag(NAlice::NVideoCommon::FLAG_DISABLE_MORDOVIA_TABS) && request.Interfaces().GetIsTvPlugged();
}

bool TrySwitchMordoviaTab(const TScenarioRunRequestWrapper& request, TScenarioHandleContext& ctx) {
    LOG_DEBUG(ctx.Ctx.Logger()) << "Trying to process open mordovia tab intent..." << Endl;
    if (!request.Input().FindSemanticFrame(SWITCH_MORDOVIA_TAB_FRAME)) {
        LOG_DEBUG(ctx.Ctx.Logger()) << "Processing open mordovia tab intent failed: no semantic frames for open mordovia tab intent!" << Endl;
        return false;
    }

    if (!ShowMordoviaTabsSupported(request)) {
        LOG_DEBUG(ctx.Ctx.Logger()) << "Processing open mordovia tab intent failed: mordovia tabs aren't supported!" << Endl;
        return false;
    }

    auto frame = TFrame::FromProto(*request.Input().FindSemanticFrame(SWITCH_MORDOVIA_TAB_FRAME));
    const TPtrWrapper<TSlot> slot = frame.FindSlot(NVideoCommon::SLOT_TAB_NAME);

    NMordoviaTabs::ETabName tabName;
    if (!slot ||
        !TryFromString<NMordoviaTabs::ETabName>(slot->Value.AsString(), tabName) ||
        !NMordoviaTabs::MORDOVIA_TAB_PATH.contains(tabName))
    {
        LOG_DEBUG(ctx.Ctx.Logger()) << "Processing open mordovia tab intent failed:  invalid tab_name slot value, "
            << (slot ? slot->Value.AsString() : " (empty)")  << Endl;
        return false;
    }

    TString host{NAlice::NVideoCommon::DEFAULT_WEBVIEW_VIDEO_HOST};
    if (const auto expHost = request.ExpFlag(NVideoCommon::FLAG_WEBVIEW_VIDEO_HOST); expHost.Defined()) {
        host = *expHost;
    }

    TStringBuf path = NMordoviaTabs::MORDOVIA_TAB_PATH.at(tabName);

    const TString pathWithCGI = UrlWithCgiParams(path, request);

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    AddWebViewCommand(request, bodyBuilder, host, pathWithCGI, /* splashDiv */ TStringBuf(), /* oldViewKey */ TStringBuf{"video"}, /* isMainPage */ false);

    NVideoCommon::FillAnalyticsInfo(bodyBuilder, ANALYTICS_SELECT_TAB_INTENT_NAME, ANALYTICS_PRODUCT_SCENARIO_NAME);

    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);

    LOG_DEBUG(ctx.Ctx.Logger()) << "Processing open mordovia tab intent succeed!" << Endl;
    return true;
}

bool TrySelectVideo(const TScenarioRunRequestWrapper& request, TScenarioHandleContext& ctx) {
    LOG_DEBUG(ctx.Ctx.Logger()) << "Trying to process mordovia video selection intent..." << Endl;
    const auto& deviceState = request.Proto().GetBaseRequest().GetDeviceState();
    if (NAlice::NVideoCommon::CurrentScreenId(deviceState) != NAlice::NVideoCommon::EScreenId::MordoviaMain) {
        LOG_DEBUG(ctx.Ctx.Logger()) << "Process mordovia video selection intent failed: wrong screen!" << Endl;
        return false;
    }

    const auto selectedVideo = TryGetSelectedVideo(request, ctx.Ctx.Logger());
    if (!selectedVideo.Defined() ||
        (selectedVideo->Get("uuid").GetString().Empty() && selectedVideo->Get("serial_id").GetString().Empty()))
    {
        LOG_DEBUG(ctx.Ctx.Logger()) << "Process mordovia video selection intent failed: "
            << (selectedVideo.Defined() ? "selected item has no uuid and no serial_id" : "nothing was selected!") << Endl;
        return false;
    }

    TStringBuf url = (*selectedVideo)["url"].GetString();
    TStringBuf uuid = (*selectedVideo)["uuid"].GetString();
    TStringBuf serialId = (*selectedVideo)["serial_id"].GetString();

    if (IsWebviewPromoUrl(url)) {
        ShowWebviewPromoUrl(ctx, request, url);
        return true;
    }

    TStringBuf requestedItemId = serialId.Empty() ? uuid : serialId;

    const THttpProxyRequest vhRequest = NVideoCommon::PrepareFrontendVhPlayerRequest(TString{requestedItemId}, request, ctx);
    AddHttpRequestItems(ctx, vhRequest, FRONTEND_VH_PLAYER_REQUEST_ITEM, FRONTEND_VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM);

    LOG_DEBUG(ctx.Ctx.Logger()) << "Processing mordovia video selection intent succeed!" << Endl;
    return true;
}

} // namespace

void TMordoviaPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    if (!request.Interfaces().GetHasMordoviaWebView()) {
        NVideoCommon::AddIrrelevantResponse(ctx);
        return;
    }

    if (TEnvironmentStateHelper{request}.IsTandemEnabledForFollower()) {
        NVideoCommon::AddIrrelevantResponse(ctx);
        return;
    }

    if (TryOpenMordovia(request, ctx)) {
        return;
    }

    if (TrySwitchMordoviaTab(request, ctx)) {
        return;
    }

    if (TrySelectVideo(request, ctx)) {
        return;
    }

    NVideoCommon::AddIrrelevantResponse(ctx);
}

} // namespace NAlice::NHollywood::NMordovia
