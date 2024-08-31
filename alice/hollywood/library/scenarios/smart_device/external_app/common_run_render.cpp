#include "common_run_render.h"
#include "common.h"
#include "web_os_helper.h"

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/frame_request_params.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/protos/data/scenario/centaur/main_screen.pb.h>
#include <alice/protos/data/scenario/centaur/webview.pb.h>
#include <alice/protos/data/scenario/data.pb.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf OPEN_EXTERNAL_APP_FRAME = "alice.open_smart_device_external_app";
constexpr TStringBuf OPEN_EXACT_EXTERNAL_APP_FRAME = "alice.open_smart_device_exact_external_app";
constexpr TStringBuf CENTAUR_COLLECT_MAIN_SCREEN_FRAME = "alice.centaur.collect_main_screen";

constexpr TStringBuf NLG_TEMPLATE_NAME = "external_app";

const TString YOUTUBE_DIV_CARD_ID = "youtube.webview";
const TString YOUTUBE_WEBVIEW_URL = "https://www.youtube.com/";
const TString YOUTUBE_PACKAGE_NAME = "com.yandex.tv.ytplayer";
const TString SCENARIO_WIDGET_MECHANICS_EXP_FLAG_NAME = "scenario_widget_mechanics";
const TString CENTAUR_TYPED_ACTION_EXP_FLAG_NAME = "centaur_typed_action";

} // namespace

const TMaybe<TString> GetRecognizedPackageName(const TFrame& frame, TRTLogger& logger) {
    const auto rawApplicationSlot = frame.FindSlot(NSmartDevice::NExternalApp::NSlots::APPLICATION_NAME);
    if (!rawApplicationSlot || !rawApplicationSlot.IsValid()) {
        LOG_ERR(logger) << "App name is not recognized";
        return Nothing();
    }
    const auto applicationSlot = rawApplicationSlot.Get();
    const TString recognizedPackageName = applicationSlot->Value.AsString();
    LOG_INFO(logger) << "App to be opened: " << recognizedPackageName;
    return recognizedPackageName;
}

void ReplyIrrelevant(
    TScenarioHandleContext& ctx, TRunResponseBuilder& builder, TResponseBodyBuilder& bodyBuilder, TNlgData& nlgData, TRTLogger& logger) {
    LOG_DEBUG(logger) << "Can not open app, return irrelevant result";
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_TEMPLATE_NAME, "open_external_app_unavailable", {}, nlgData);
    builder.SetIrrelevant();
    ctx.ServiceCtx.AddProtobufItem(*(std::move(builder).BuildResponse()), RESPONSE_ITEM);
}

void AddRenderDataToContext(TScenarioHandleContext& ctx, TResponseBodyBuilder& bodyBuilder) {
    for (auto const& [cardId, cardData] : bodyBuilder.GetRenderData()) {
        LOG_INFO(ctx.Ctx.Logger()) << "Adding render_data (" << cardId << ") to context";
        ctx.ServiceCtx.AddProtobufItem(cardData, RENDER_DATA_ITEM);
    }
}

EOpenExternalAppResult MakeSendAndroidAppIntentDirective(
    const TScenarioRunRequestWrapper& request, TRTLogger& logger, TNlgData& nlgData,
    const TString& recognizedPackageName, TMaybe<NScenarios::TDirective>& maybeDirective)
{
        const auto& packageState = request.BaseRequestProto().GetDeviceState().GetPackagesState();
        bool hasTvStore = request.BaseRequestProto().GetInterfaces().GetHasTvStore();
        TDirective directive;
        auto& sendAndroidIntentDirective = *directive.MutableSendAndroidAppIntentDirective();
        sendAndroidIntentDirective.SetStartType(NAlice::NScenarios::TSendAndroidAppIntentDirective_EIntentStartType_StartActivity);
        TSendAndroidAppIntentDirective_TIntentFlags& flags = *sendAndroidIntentDirective.MutableFlags();
        flags.SetFlagActivityNewTask(true);

        auto& component = *sendAndroidIntentDirective.MutableComponent();
        for (const auto& package : packageState.GetInstalled()) {
            const auto& packageName = package.GetPackageInfo().GetName();
            if (recognizedPackageName == packageName) {
                sendAndroidIntentDirective.SetCategory("android.intent.category.LAUNCHER");
                sendAndroidIntentDirective.SetAction("android.intent.action.MAIN");

                nlgData.Context["application"] = package.GetPackageInfo().GetHumanReadableName();

                component.set_pkg(packageName);
                component.set_cls(package.GetMainActivity());
                maybeDirective = directive;
                return EOpenExternalAppResult::OpenRequestedApp;
            }
        }
        if (!hasTvStore) {
            LOG_DEBUG(logger) << "Application not recognized, there is no store on device";
            return EOpenExternalAppResult::UnableOpenRequestedApp;
        }

        sendAndroidIntentDirective.SetAction("android.intent.action.VIEW");
        sendAndroidIntentDirective.SetUri("home-app://market_item?package=" + recognizedPackageName);
        maybeDirective = directive;
        return EOpenExternalAppResult::OpenRequestedAppStorePage;
}

EOpenExternalAppResult FillResponseDirective(
    const TScenarioRunRequestWrapper& request, TRTLogger& logger, TNlgData& nlgData,
    const TFrame& frame, TMaybe<TDirective>& maybeDirective, bool canLaunchWebOSApp)
{
    const TMaybe<TString>& maybeRecognizedPackageName = GetRecognizedPackageName(frame, logger);
    if (!maybeRecognizedPackageName.Defined()) {
        return EOpenExternalAppResult::UnableOpenRequestedApp;
    }
    const TString& recognizedPackageName = maybeRecognizedPackageName.GetRef();
    if (recognizedPackageName.StartsWith("unlisted")) {
        return EOpenExternalAppResult::UnableOpenUnlistedApp;
    }

    if (canLaunchWebOSApp) {
        LOG_INFO(logger) << "Making WebOSLaunchAppDirective";
        return MakeWebOSLaunchAppDirective(request, logger, recognizedPackageName, maybeDirective);
    } else {
        LOG_INFO(logger) << "Making SendAndroidAppIntentDirective";
        return MakeSendAndroidAppIntentDirective(request, logger, nlgData, recognizedPackageName, maybeDirective);
    }
}

void AddMediaSessionAction(TActionSpace& actionSpace, TActionSpace_TAction& action, const TString& actionId, const TString& semanticFrameName) {
    auto& analytics = *action.MutableSemanticFrame()->MutableAnalytics();
    analytics.SetProductScenario(NAlice::NProductScenarios::SMART_DEVICE_EXTERNAL_APP);
    analytics.SetPurpose(actionId);
    analytics.SetOrigin(TAnalyticsTrackingModule_EOrigin_SmartSpeaker);

    auto& nluHint = *actionSpace.AddNluHints();
    nluHint.SetActionId(actionId);
    nluHint.SetSemanticFrameName(semanticFrameName);

    (*actionSpace.MutableActions())[actionId] = action;
}

void AddShowView(
    TScenarioHandleContext& ctx, const TFrame& frame, TRunResponseBuilder& builder, TResponseBodyBuilder& bodyBuilder, TNlgData& nlgData, TRTLogger& logger) {
    const TMaybe<TString> &maybeRecognizedPackage = GetRecognizedPackageName(frame, logger);
    if (maybeRecognizedPackage.Defined() && maybeRecognizedPackage.GetRef() == YOUTUBE_PACKAGE_NAME) {
        NRenderer::TDivRenderData divRendererData;
        divRendererData.SetCardId(YOUTUBE_DIV_CARD_ID);
        auto& webviewData = *divRendererData.MutableScenarioData()->MutableCentaurWebviewData();
        webviewData.SetId(YOUTUBE_DIV_CARD_ID);
        webviewData.SetWebviewUrl(YOUTUBE_WEBVIEW_URL);
        webviewData.SetShowNavigationBar(true);
        webviewData.SetMediaSessionId(YOUTUBE_DIV_CARD_ID);
        ctx.ServiceCtx.AddProtobufItem(divRendererData, RENDER_DATA_ITEM);

        TDirective directive;
        auto& showViewDirective = *directive.MutableShowViewDirective();
        showViewDirective.SetName("show_view");
        showViewDirective.SetCardId(YOUTUBE_DIV_CARD_ID);
        showViewDirective.MutableLayer()->MutableContent();
        showViewDirective.SetDoNotShowCloseButton(true);
        showViewDirective.SetActionSpaceId(YOUTUBE_DIV_CARD_ID);
        showViewDirective.SetInactivityTimeout(TShowViewDirective_EInactivityTimeout_Infinity);
        bodyBuilder.AddDirective(std::move(directive));

        TActionSpace actionSpace;
        {
            TActionSpace_TAction action;
            action.MutableSemanticFrame()->MutableTypedSemanticFrame()->MutableMediaSessionPlaySemanticFrame()->MutableMediaSessionId()->SetStringValue(YOUTUBE_DIV_CARD_ID);
            AddMediaSessionAction(actionSpace, action, "youtube_mediasession_play", "personal_assistant.scenarios.player.continue");
        }
        {
            TActionSpace_TAction action;
            action.MutableSemanticFrame()->MutableTypedSemanticFrame()->MutableMediaSessionPauseSemanticFrame()->MutableMediaSessionId()->SetStringValue(YOUTUBE_DIV_CARD_ID);
            AddMediaSessionAction(actionSpace, action, "youtube_mediasession_pause", "personal_assistant.scenarios.player.pause");
        }
        bodyBuilder.AddActionSpace(YOUTUBE_DIV_CARD_ID, actionSpace);

        ctx.ServiceCtx.AddProtobufItem(*(std::move(builder).BuildResponse()), RESPONSE_ITEM);
    }
    else {
        ReplyIrrelevant(ctx, builder, bodyBuilder, nlgData, logger);
    }
}

void PrepareWidgetCard(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, TRunResponseBuilder& builder, TResponseBodyBuilder& bodyBuilder) {
    NData::TScenarioData scenarioData;
    auto& widgetData = *scenarioData.MutableCentaurScenarioWidgetData();
    widgetData.SetWidgetType("webview");
    auto& cardData = *widgetData.AddWidgetCards();
    cardData.MutableYouTubeCardData();
    
    if (request.HasExpFlag(CENTAUR_TYPED_ACTION_EXP_FLAG_NAME)) {
        TTypedSemanticFrame tsf;
        auto* frame = tsf.MutableOpenSmartDeviceExternalAppFrame();
        frame->MutableApplication()->SetExternalAppValue(YOUTUBE_PACKAGE_NAME);

        google::protobuf::Any typedAction;
        typedAction.PackFrom(std::move(tsf));
        cardData.MutableTypedAction()->CopyFrom(typedAction);
    } else {
        TFrameAction onClickAction;
        auto* parsedUtterance = onClickAction.MutableParsedUtterance();
        auto* frame = parsedUtterance->MutableTypedSemanticFrame()->MutableOpenSmartDeviceExternalAppFrame();
        frame->MutableApplication()->SetExternalAppValue(YOUTUBE_PACKAGE_NAME);
        parsedUtterance->MutableParams()->SetDisableOutputSpeech(true);
        parsedUtterance->MutableParams()->SetDisableShouldListen(true);

        auto* analytics = parsedUtterance->MutableAnalytics();
        analytics->SetProductScenario("CentaurMainScreen");
        analytics->SetPurpose("open_youtube");
        analytics->SetOrigin(TAnalyticsTrackingModule_EOrigin_SmartSpeaker);

        const TString frameActionId = "OnClickMainScreenYouTubeCard";
        cardData.SetAction("@@mm_deeplink#" + frameActionId);
        bodyBuilder.AddAction(frameActionId, std::move(onClickAction));
    }

    bodyBuilder.AddScenarioData(scenarioData);

    ctx.ServiceCtx.AddProtobufItem(*(std::move(builder).BuildResponse()), RESPONSE_ITEM);
}

void AddResponseItems(
        TRunResponseBuilder& builder, TResponseBodyBuilder &bodyBuilder, TNlgData& nlgData, const bool isIrrelevant,
        const TString templateName, TMaybe<TDirective>& maybeDirective) {
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_TEMPLATE_NAME, templateName, {}, nlgData);
    if (isIrrelevant) {
        builder.SetIrrelevant();
    } else if (maybeDirective.Defined()) {
        TDirective directive = maybeDirective.GetRef();
        bodyBuilder.AddDirective(std::move(directive));
    }
}

void TCommonrunRenderHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    if (request.HasExpFlag(SCENARIO_WIDGET_MECHANICS_EXP_FLAG_NAME)) {
        if (request.Input().FindSemanticFrame(CENTAUR_COLLECT_MAIN_SCREEN_FRAME)) {
            PrepareWidgetCard(ctx, request, builder, bodyBuilder);
            return;
        }
    }

    TNlgData nlgData{logger, request};
    const auto sframe = request.Input().FindSemanticFrame(OPEN_EXTERNAL_APP_FRAME);
    const auto exact_sframe = request.Input().FindSemanticFrame(OPEN_EXACT_EXTERNAL_APP_FRAME);
    bool canLaunchWebOSApp = CanLaunchWebOSAppDirective(request);
    if (!(sframe || exact_sframe) || (!request.BaseRequestProto().GetInterfaces().GetCanHandleAndroidAppIntent() && !canLaunchWebOSApp)) {
        ReplyIrrelevant(ctx, builder, bodyBuilder, nlgData, logger);
        return;
    }

    const TString analiticsActionId = "open_external_app";
    auto& analyticsInfoBuilder = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::SMART_DEVICE_EXTERNAL_APP);
    analyticsInfoBuilder.AddAction(analiticsActionId, analiticsActionId, "");
    TFrame frame = exact_sframe ? TFrame::FromProto(*exact_sframe) : TFrame::FromProto(*sframe);

    if (request.ClientInfo().IsCentaur()) {
        AddShowView(ctx, frame, builder, bodyBuilder, nlgData, logger);
        return;
    }

    TMaybe<TDirective> maybeDirective;

    const EOpenExternalAppResult fillDirectiveResult = FillResponseDirective(request, logger, nlgData, frame, maybeDirective, canLaunchWebOSApp);
    if (fillDirectiveResult == EOpenExternalAppResult::UnableOpenRequestedApp) {
        LOG_DEBUG(logger) << "Application is not recognized, return irrelevant result";
        AddResponseItems(builder, bodyBuilder, nlgData, true, "render_failed_result", maybeDirective);
    } else if (fillDirectiveResult == EOpenExternalAppResult::OpenRequestedAppStorePage) {
        if (!request.BaseRequestProto().GetInterfaces().GetHasTvStore()) {
            LOG_DEBUG(logger) << "Can not open application, no store available, return irrelevant result";
            AddResponseItems(builder, bodyBuilder, nlgData, false, "open_external_app_unavailable_no_store", maybeDirective);
        } else {
            LOG_DEBUG(logger) << "Application recognized, but not installed, ask client to open store";
            AddResponseItems(builder, bodyBuilder, nlgData, false, "render_store_success_result", maybeDirective);
        }
    } else if (fillDirectiveResult == EOpenExternalAppResult::OpenRequestedApp) {
        LOG_DEBUG(logger) << "Application recognized and installed. Ask client to open requested app";
        AddResponseItems(builder, bodyBuilder, nlgData, false, "render_success_result", maybeDirective);
    } else if (fillDirectiveResult == EOpenExternalAppResult::UnableOpenUnlistedApp) {
        LOG_DEBUG(logger) << "Application recognized but unlisted.";
        AddResponseItems(builder, bodyBuilder, nlgData, false, "open_external_app_unavailable_unlisted", maybeDirective);
    } else {
        LOG_DEBUG(logger) << "Unexpected error occurred";
        AddResponseItems(builder, bodyBuilder, nlgData, true, "common_error", maybeDirective);
    }

    AddRenderDataToContext(ctx, bodyBuilder);

    ctx.ServiceCtx.AddProtobufItem(*(std::move(builder).BuildResponse()), RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood
