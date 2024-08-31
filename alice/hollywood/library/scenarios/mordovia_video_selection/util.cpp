#include "util.h"

#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/mordovia_webview_defs.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>

namespace {

void ParseUriString(const TStringBuf uri, TStringBuf& path, TCgiParameters& cgiParams) {
    TStringBuf cgiString;
    if (uri.TrySplit('?', path, cgiString)) {
        cgiParams.Scan(cgiString);
    } else {
        path = uri;
    }
}

}   // namespace

namespace NAlice::NHollywood::NMordovia {

TStringBuf GetCurrentViewKey(const TScenarioRunRequestWrapper& request) {
    return NVideoCommon::GetCurrentViewKey(request.Proto().GetBaseRequest().GetDeviceState());
}

TString PrepareWebviewUrlForPromo(const TStringBuf originalPromoUrl, const TScenarioRunRequestWrapper request) {
    TCgiParameters originalCgi;
    TStringBuf originalPath;
    ParseUriString(originalPromoUrl, originalPath, originalCgi);

    TCgiParameters cgi = NVideoCommon::GetDefaultWebviewCgiParams(request);
    for (const auto& cgiParam: originalCgi) {
        if (!cgi.Has(cgiParam.first)) {
            cgi.InsertUnescaped(cgiParam.first, cgiParam.second);
        }
    }
    TStringBuilder res;
    res << originalPath << "?" << cgi.Print();
    return res;
}

void AddWebViewCommand(const TScenarioRunRequestWrapper& request, TResponseBodyBuilder& bodyBuilder,
                       TStringBuf url, TStringBuf splashDiv, TStringBuf oldViewKey, bool isMainPage) {
    TString host = TStringBuilder() << url.NextTok("://") << "://" << url.NextTok("/"); // grab scheme && host
    TString path = TStringBuilder() << "/" << url; // path remains

    AddWebViewCommand(request, bodyBuilder, host, path, splashDiv, oldViewKey, isMainPage);
}

void AddWebViewCommand(const TScenarioRunRequestWrapper& request, TResponseBodyBuilder& bodyBuilder,
                       TStringBuf host, TStringBuf path, TStringBuf splashDiv,
                       TStringBuf oldViewKey, bool isMainPage) {
    // remove oldViewKey when "FLAG_VIDEO_MORDOVIA_SPA" appears on prod
    TStringBuf currentViewKey = GetCurrentViewKey(request);
    TString directiveName;
    NJson::TJsonValue payload;

    // We will stay in the same view_key in two cases:
    // 1) we are about to show Mordovia-main page && current view_key is main view_key
    // 2) we are about to show any other video-related page && current view_key is either main view_key or video view_key
    // In any other case we will create a new view_key via mordovia_show directive
    bool sameViewKey = NVideoCommon::IsWebViewMainViewKey(currentViewKey) || !isMainPage && NVideoCommon::IsWebViewVideoViewKey(currentViewKey);
    if (!request.HasExpFlag(NVideoCommon::FLAG_DISABLE_VIDEO_MORDOVIA_COMMAND_NAVIGATION) && sameViewKey) {
        directiveName = MORDOVIA_COMMAND_DIRECTIVE_NAME;
        payload = NVideoCommon::BuildChangePathCommandJsonPayload(currentViewKey, TString{path}, /* clearNavigationHistory */ isMainPage);
    } else {
        directiveName = MORDOVIA_SHOW_DIRECTIVE_NAME;
        TStringBuf viewKey = oldViewKey;
        if (!request.HasExpFlag(NVideoCommon::FLAG_DISABLE_VIDEO_MORDOVIA_SPA)) {
            viewKey = isMainPage ? NVideoCommon::VIDEO_STATION_SPA_MAIN_VIEW_KEY : NVideoCommon::VIDEO_STATION_SPA_VIDEO_VIEW_KEY;
        }
        payload = NVideoCommon::BuildMordoviaShowJsonPayload(viewKey, /* url */ TStringBuilder() << host << path, TString{splashDiv}, /* doGoBack */ isMainPage);
    }

    bodyBuilder.AddClientActionDirective(directiveName, payload);
    bodyBuilder.AddClientActionDirective(TString(NVideoCommon::COMMAND_TTS_PLAY_PLACEHOLDER), NJson::TJsonValue());
}

bool IsSilentResponse(const TScenarioRunRequestWrapper& request) {
    if (const auto* callback = request.Input().GetCallback()) {
        NVideoCommon::TMordoviaJsCallbackPayload jsCallbackPayload(callback->GetPayload());
        if (jsCallbackPayload.Command() == SELECT_VIDEO_CALLBACK ||
            jsCallbackPayload.Command() == SELECT_VIDEO_CALLBACK_DEPRECATED)
        {
            return true;
        }
    }
    if (const auto semanticFrame = request.Input().FindSemanticFrame(VIDEO_SELECTION_BY_REMOTE_CONTROL_FRAME)) {
        auto frame = TFrame::FromProto(*semanticFrame);
        if (const auto slot = frame.FindSlot(NAlice::NVideoCommon::SLOT_SILENT_RESPONSE)) {
            if (auto silentResponse = slot->Value.As<bool>()) {
                return *silentResponse;
            }
        }
    }
    return false;
}

} // namespace NAlice::NHollywood::NMordovia
