#include "render_handle.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/mordovia_video_selection/util.h>

#include <alice/library/video_common/age_restriction.h>
#include <alice/library/video_common/frontend_vh_helpers/util.h>
#include <alice/library/video_common/frontend_vh_helpers/video_item_helper.h>
#include <alice/library/video_common/mordovia_webview_defs.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>
#include <alice/library/video_common/video_helper.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NMordovia {

namespace {

NAlice::NVideoCommon::ESelectionAction GetAction(const NVideoCommon::TVideoItemHelper& videoItemHelper,
                                                 const TScenarioRunRequestWrapper& request) {


    if (const auto* callback = request.Input().GetCallback()) {
        NVideoCommon::TMordoviaJsCallbackPayload jsCallbackPayload(callback->GetPayload());
        if (jsCallbackPayload.Command() == SELECT_VIDEO_CALLBACK ||
            jsCallbackPayload.Command() == SELECT_VIDEO_CALLBACK_DEPRECATED)
        {
            const auto videoAction = jsCallbackPayload.Payload().TrySelect("action").GetString();
            if (videoAction) {
                NAlice::NVideoCommon::ESelectionAction action;
                return (TryFromString<NAlice::NVideoCommon::ESelectionAction>(videoAction, action) ?
                    action : NAlice::NVideoCommon::ESelectionAction::Play);
            }
        }
    }

    if (!request.Input().FindSemanticFrame(VIDEO_SELECTION_FRAME) &&
        !request.Input().FindSemanticFrame(VIDEO_SELECTION_BY_REMOTE_CONTROL_FRAME))
    {
        if (videoItemHelper.HasDescription()) {
            return NAlice::NVideoCommon::ESelectionAction::Description;
        }
        return NAlice::NVideoCommon::ESelectionAction::Play;
    }

    const auto frame = request.Input().FindSemanticFrame(VIDEO_SELECTION_FRAME) ?
            request.Input().CreateRequestFrame(VIDEO_SELECTION_FRAME) :
            request.Input().CreateRequestFrame(VIDEO_SELECTION_BY_REMOTE_CONTROL_FRAME);
    const auto actionSlot = frame.FindSlot(NAlice::NVideoCommon::SLOT_ACTION);
    if (!actionSlot || actionSlot->Type != NAlice::NVideoCommon::SLOT_CUSTOM_SELECTION_ACTION_TYPE) {
        return NAlice::NVideoCommon::ESelectionAction::Play;
    }
    NAlice::NVideoCommon::ESelectionAction action = NAlice::NVideoCommon::ESelectionAction::Play;
    TryFromString<NAlice::NVideoCommon::ESelectionAction>(actionSlot->Value.AsString(), action);
    if (action == NAlice::NVideoCommon::ESelectionAction::Description && !videoItemHelper.HasDescription()) {
        action = NAlice::NVideoCommon::ESelectionAction::Play;
    }
    return action;
}

void ShowDescription(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                     TResponseBodyBuilder& bodyBuilder, const NVideoCommon::TVideoItemHelper& videoItemHelper,
                     TStringBuf attention) {

    TNlgData nlgData(ctx.Ctx.Logger(), request);
    nlgData.AddAttention(attention);
    if (!IsSilentResponse(request)) {
        bodyBuilder.AddRenderedTextWithButtonsAndVoice("mordovia", "render_result", /* buttons = */ {}, nlgData);
    }

    if (!request.HasExpFlag(NAlice::NVideoCommon::FLAG_DISABLE_VIDEO_WEBVIEW_VIDEO_ENTITY) && !videoItemHelper.GetOntoId().Empty()) {
        TCgiParameters cgi = NVideoCommon::GetDefaultWebviewCgiParams(request);
        cgi.InsertUnescaped(NVideoCommon::WEBVIEW_PARAM_ENTREF, NVideoCommon::MakeEntref(videoItemHelper.GetOntoId()));

        if (!request.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_WEBVIEW_DO_NOT_ADD_UUIDS)) {
            cgi.InsertUnescaped(NVideoCommon::WEBVIEW_PARAM_UUIDS, videoItemHelper.GetUuid());
        }

        TString host = ToString(NAlice::NVideoCommon::DEFAULT_WEBVIEW_VIDEO_HOST);
        if (const auto expHost = request.ExpFlag(NAlice::NVideoCommon::FLAG_WEBVIEW_VIDEO_HOST); expHost.Defined()) {
            host = *expHost;
        }
        TString path = ToString(NAlice::NVideoCommon::DEFAULT_VIDEO_SINGLE_CARD_PATH);
        if (const auto expPath = request.ExpFlag(NAlice::NVideoCommon::FLAG_WEBVIEW_VIDEO_ENTITY_PATH); expPath.Defined()) {
            path = *expPath;
        }
        TString splashDiv = ToString(NAlice::NVideoCommon::DEFAULT_VIDEO_SINGLE_CARD_SPLASH);
        if (const auto expSplash = request.ExpFlag(NAlice::NVideoCommon::FLAG_WEBVIEW_VIDEO_ENTITY_SPLASH); expSplash.Defined()) {
            splashDiv = *expSplash;
        }

        const auto& delimiter = path.Contains("?") ? "&" : "?";
        TString pathWithParams = TStringBuilder() << path << delimiter << cgi.Print();
        AddWebViewCommand(request, bodyBuilder, host, pathWithParams, splashDiv, /* oldViewKey */ TStringBuf{"video"});
        return;
    }
    TDirective oneOfDirective;
    *oneOfDirective.MutableShowVideoDescriptionDirective() = videoItemHelper.MakeShowVideoDescriptionDirective();
    bodyBuilder.AddDirective(std::move(oneOfDirective));

    bodyBuilder.AddClientActionDirective(TString(NVideoCommon::COMMAND_TTS_PLAY_PLACEHOLDER), NJson::TJsonValue());
}

void PlayVideo(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
               TResponseBodyBuilder& bodyBuilder, const NVideoCommon::TVideoItemHelper& videoItemHelper) {
    TDirective oneOfDirective;
    *oneOfDirective.MutableVideoPlayDirective() = videoItemHelper.MakeVideoPlayDirective(request);
    bodyBuilder.AddDirective(std::move(oneOfDirective));
    TNlgData nlgData(ctx.Ctx.Logger(), request);
    nlgData.AddAttention(NAlice::NVideoCommon::ATTENTION_AUTOPLAY);
    if (!IsSilentResponse(request)) {
        bodyBuilder.AddRenderedTextWithButtonsAndVoice("mordovia", "render_result", /* buttons = */ {}, nlgData);
    }
}

} // namespace

void TMordoviaRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto vhResponse =
        RetireHttpResponseJson(ctx, FRONTEND_VH_PLAYER_RESPONSE_ITEM, FRONTEND_VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM);
    auto videoItemHelper = NVideoCommon::TVideoItemHelper::TryMakeFromVhPlayerResponse(vhResponse);
    if (!videoItemHelper.Defined()) {
        NVideoCommon::AddIrrelevantResponse(ctx);
        return;
    }

    auto ottResponse = RetireHttpResponseJsonMaybe(ctx, OTT_STREAMS_META_RESPONSE_ITEM, OTT_STREAMS_META_REQUEST_RTLOG_TOKEN_ITEM);
    if (ottResponse) {
        videoItemHelper->AddSubtitlesAndAudioTracks(std::move(*ottResponse));
    }

    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    NAlice::NVideoCommon::TAgeRestrictionCheckerParams params;
    params.MinAge = videoItemHelper->GetAgeRestriction();
    params.RestrictionLevel = GetContentRestrictionLevel(request.ContentRestrictionLevel());

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    NVideoCommon::FillAnalyticsInfo(bodyBuilder, ANALYTICS_SELECT_VIDEO_INTENT_NAME, ANALYTICS_PRODUCT_SCENARIO_NAME);

    switch (GetAction(*videoItemHelper, request)) {
        case NVideoCommon::ESelectionAction::Play: {
            if (!NAlice::NVideoCommon::PassesAgeRestriction(params)) {
                TNlgData nlgData(ctx.Ctx.Logger(), request);
                nlgData.AddAttention(NVideoCommon::ATTENTION_ALL_RESULTS_FILTERED);
                bodyBuilder.AddRenderedTextWithButtonsAndVoice("mordovia", "render_result", /* buttons = */ {}, nlgData);
            } else {
                if (videoItemHelper->GetIsPaidContent() && !videoItemHelper->GetHasActiveLicense()) {
                    ShowDescription(ctx, request, bodyBuilder, *videoItemHelper,
                                    NAlice::NVideoCommon::ATTENTION_PAID_CONTENT);
                } else {
                    PlayVideo(ctx, request, bodyBuilder, *videoItemHelper);
                }
            }
            break;
        }
        case NVideoCommon::ESelectionAction::Description: {
            if (!NAlice::NVideoCommon::PassesAgeRestriction(params)) {
                TNlgData nlgData(ctx.Ctx.Logger(), request);
                nlgData.AddAttention(NVideoCommon::ATTENTION_ALL_RESULTS_FILTERED);
                bodyBuilder.AddRenderedTextWithButtonsAndVoice("mordovia", "render_result", /* buttons = */ {}, nlgData);
            } else {
                ShowDescription(ctx, request, bodyBuilder, *videoItemHelper, NAlice::NVideoCommon::ATTENTION_AUTOSELECT);
            }
            break;
        }
        case NVideoCommon::ESelectionAction::ListSeasons: {
            NVideoCommon::AddIrrelevantResponse(ctx);
            return;
        }
        case NVideoCommon::ESelectionAction::ListEpisodes: {
            NVideoCommon::AddIrrelevantResponse(ctx);
            return;
        }
    }

    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NMordovia
