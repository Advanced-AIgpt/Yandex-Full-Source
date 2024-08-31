#include "renderer.h"
#include "names.h"

namespace NAlice::NHollywood::NShowTrafficBass {

TRenderer::TRenderer(TScenarioHandleContext ctx, const TScenarioRunRequestWrapper& runRequest, const TFrame* frame)
    : Logger{ctx.Ctx.Logger()}
    , RunRequest{runRequest}
    , NlgWrapper(TNlgWrapper::Create(ctx.Ctx.Nlg(), RunRequest, ctx.Rng, ctx.UserLang))
    , Builder{&NlgWrapper}
    , NlgData{Logger, runRequest}
    , Frame{frame}
{
    if (frame) {
        SetProductScenarioName(NShowTrafficBass::PRODUCT_SCENARIO_NAME);
        SetIntentName(frame->Name());
    }
}

google::protobuf::Any* TRenderer::GetMutableState() {
    return BodyBuilder().GetResponseBody().MutableState();
}

TRunResponseBuilder& TRenderer::GetBuilder() {
    return Builder;
}

void TRenderer::SetProductScenarioName(const TString& scenarioName) {
    AnalyticsInfoBuilder().SetProductScenarioName(scenarioName);
}

void TRenderer::SetIntentName(const TString& intentName) {
    AnalyticsInfoBuilder().SetIntentName(intentName);
    Builder.GetMutableFeatures().SetIntent(intentName);
}

void TRenderer::SetShouldListen(bool shouldListen) {
    BodyBuilder().SetShouldListen(shouldListen);
}

void TRenderer::AddFeedBackSuggests() {
    if (!RunRequest.Interfaces().GetSupportsFeedback()) {
        return;
    }

    auto* layout = BodyBuilder().GetResponseBody().MutableLayout();
    for (const TString& feedback : {NFeedbackOptions::NEGATIVE, NFeedbackOptions::POSITIVE}) {
        AddSpecialSuggest(feedback);

        // the new suggest is the last suggest, so we move it to the beginning
        for (int suggestNumber = layout->SuggestButtonsSize() - 1; suggestNumber - 1 >= 0; --suggestNumber) {
            layout->MutableSuggestButtons()->SwapElements(suggestNumber, suggestNumber - 1);
        }
    }

    AddSpecialSuggestAction(CALLBACK_FEEDBACK_NAME, NFeedbackOptions::POSITIVE);
    AddSpecialSuggestAction(CALLBACK_FEEDBACK_NAME, NFeedbackOptions::NEGATIVE, NShowTrafficBass::FEEDBACK_OPTIONS_REASONS);
}

void TRenderer::AddOpenUriDirective(const TString& uri, const TString& name) {
    if (RunRequest.ClientInfo().IsSmartSpeaker())
        return;
    NScenarios::TDirective directive;
    NScenarios::TOpenUriDirective& openUriDirective = *directive.MutableOpenUriDirective();
    openUriDirective.SetName(name);
    openUriDirective.SetUri(uri);
    BodyBuilder().AddDirective(std::move(directive));
}

void TRenderer::AddTypeTextSuggest(TStringBuf type) {
    TString actionId = TString::Join("suggest_", type);

    TString suggestUtterance = NlgWrapper.RenderPhrase(FEEDBACK,
                                                       /* phraseName= */ TString::Join("render_suggest_utterance__", type),
                                                       NlgData).Text;
    TString suggestCaption = NlgWrapper.RenderPhrase(FEEDBACK,
                                                     /* phraseName= */ TString::Join("render_suggest_caption__", type),
                                                     NlgData).Text;

    NScenarios::TFrameAction action;

    NScenarios::TDirective directive;
    NScenarios::TTypeTextDirective* typeTextDirective = directive.MutableTypeTextDirective();
    typeTextDirective->SetText(suggestUtterance);
    typeTextDirective->SetName("type");
    *action.MutableDirectives()->AddList() = directive;

    BodyBuilder().AddAction(actionId, std::move(action));
    BodyBuilder().AddActionSuggest(actionId).Title(suggestCaption);
}

void TRenderer::Render(TStringBuf templateName) {
    SetShouldListen(templateName != NShowTrafficBass::SHOW_TRAFFIC_BASS_DETAILS_NLG);
    BodyBuilder().AddRenderedTextWithButtonsAndVoice(templateName,
                                                     /* phraseName= */ "render_result",
                                                     /* buttons= */ {},
                                                     NlgData);
    AddFeedBackSuggests();
}

void TRenderer::RenderFeedbackAnswer(const NScenarios::TCallbackDirective* callback) {
    const auto& payloadFields = callback->GetPayload().fields();

    SetShouldListen(/* shouldListen= */ true);

    SetProductScenarioName(TString{NShowTrafficBass::FEEDBACK});

    if (payloadFields.count("type")) {
        const TString type = payloadFields.at("type").string_value();
        LOG_INFO(Logger) << "Feedback type: " << type.Quote();

        if (type == NFeedbackOptions::POSITIVE) {
            SetIntentName(POSITIVE_FEEDBACK_INTENT);
        } else if (type == NFeedbackOptions::NEGATIVE) {
            SetIntentName(NEGATIVE_FEEDBACK_INTENT);
        } else {
            SetIntentName(CONTINUE_FEEDBACK_INTENT);
        }

        BodyBuilder().AddRenderedTextWithButtonsAndVoice(FEEDBACK,
            TString::Join("render_", type),
            /* buttons= */ {},
            NlgData);

        if (type == NFeedbackOptions::NEGATIVE && payloadFields.count("additional_data")) {
            const TString feedbacks = payloadFields.at("additional_data").string_value();
            TVector<TString> buttons;
            Split(feedbacks, ";", buttons);
            for (const TString& button : buttons) {
                AddSpecialSuggest(button);
                AddSpecialSuggestAction(CALLBACK_FEEDBACK_NAME, button);
            }
        }

        AddTypeTextSuggest(ONBOARDING_SUGGEST);
    }
}

TResponseBodyBuilder& TRenderer::BodyBuilder() {
    auto* bodyBuilder = Builder.GetResponseBodyBuilder();
    return bodyBuilder ? *bodyBuilder : Builder.CreateResponseBodyBuilder(Frame);
}

NAlice::NScenarios::IAnalyticsInfoBuilder& TRenderer::AnalyticsInfoBuilder() {
    auto& bodyBuilder = BodyBuilder();
    return bodyBuilder.HasAnalyticsInfoBuilder() ? bodyBuilder.GetAnalyticsInfoBuilder()
        : bodyBuilder.CreateAnalyticsInfoBuilder();
}

void TRenderer::AddSpecialSuggest(TStringBuf type) {
    const auto& suggestPhrase = Builder.GetNlgWrapper().RenderPhrase(NShowTrafficBass::FEEDBACK,
                                                                     /* phraseName= */ TString::Join("render_suggest_", type),
                                                                     NlgData).Text;
    const TString actionId = TString::Join("suggest_", type);

    BodyBuilder().AddActionSuggest(actionId).Title(suggestPhrase);
}

void TRenderer::AddSpecialSuggestAction(TStringBuf name, TStringBuf type, TStringBuf data) {
    NAlice::NScenarios::TFrameAction action;
    NAlice::NScenarios::TCallbackDirective* callback = action.MutableCallback();
    callback->SetName(TString{name});
    auto& payloadFields = *callback->MutablePayload()->mutable_fields();

    ::google::protobuf::Value suggestTypePB;
    suggestTypePB.set_string_value(TString(type));
    payloadFields["type"] = suggestTypePB;

    if (!data.empty()) {
        ::google::protobuf::Value dataPB;
        dataPB.set_string_value(TString{data});
        payloadFields["additional_data"] = dataPB;
    }

    TString actionId = TString::Join("suggest_", type);
    BodyBuilder().AddAction(std::move(actionId), std::move(action));
}

void TRenderer::AddScenarioData(const NData::TScenarioData& scenarioData) {
    BodyBuilder().AddScenarioData(std::move(scenarioData));
}

} // namespace NAlice::NHollywood::NShowTrafficBass
