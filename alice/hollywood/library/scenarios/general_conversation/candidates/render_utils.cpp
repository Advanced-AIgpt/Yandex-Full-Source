#include "render_utils.h"

#include <alice/hollywood/library/response/push.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/scled_animations/scled_animations_builder.h>
#include <alice/library/scled_animations/scled_animations_directive_hw.h>

#include <alice/protos/data/language/language.pb.h>
#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/data/scenario/general_conversation/conversation.pb.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

void AddGcCallback(const TString& actionId, const TString& renderedPhrase, TResponseBodyBuilder& bodyBuilder) {
    TFrameAction action;
    auto& nluHint = *action.MutableNluHint();
    nluHint.SetFrameName(actionId);

    TNluPhrase& nluPhrase = *nluHint.AddInstances();
    nluPhrase.SetLanguage(ELang::L_RUS);
    nluPhrase.SetPhrase(renderedPhrase);

    NAlice::TSemanticFrame semanticFrame;
    semanticFrame.SetName(TString{FRAME_GC_SUGGEST});

    auto& slot = *semanticFrame.AddSlots();
    slot.SetName(TString{SLOT_SUGGEST_TEXT});
    slot.SetType("string");
    slot.SetValue(renderedPhrase);

    *action.MutableCallback() = ToCallback(semanticFrame);

    bodyBuilder.AddAction(actionId, std::move(action));
}

void AddTypeTextDirectiveToAction(TFrameAction& action, const TString& text, const TString& name) {
    auto& typeTextDirective = *action.MutableDirectives()->AddList()->MutableTypeTextDirective();
    typeTextDirective.SetText(text);
    typeTextDirective.SetName(name);
}

}

void AddSuggest(const TString& actionId, const TString& renderedPhrase, const TString& analyticsType,
                bool forceGcResponse, TResponseBodyBuilder& bodyBuilder)
{
    TFrameAction action;
    AddTypeTextDirectiveToAction(action, renderedPhrase, analyticsType);

    bodyBuilder.AddAction(actionId, std::move(action));
    bodyBuilder.AddActionSuggest(actionId).Title(renderedPhrase);

    if (forceGcResponse) {
        AddGcCallback(actionId + "_callback", renderedPhrase, bodyBuilder);
    }
}

void AddSearchSuggest(const TString& renderedPhrase, TResponseBodyBuilder& bodyBuilder) {
    bodyBuilder.AddSearchSuggest().Title(renderedPhrase).Query(renderedPhrase);
}

void AddShowView(
    NAlice::TRTLogger& logger,
    TResponseBodyBuilder& bodyBuilder) {

    LOG_INFO(logger) << "Adding ShowView Directive";
    NRenderer::TDivRenderData renderData;
    renderData.SetCardId("general_conversation.scenario.div.card");
    renderData.MutableScenarioData()->MutableConversationData();
    bodyBuilder.AddShowViewDirective(std::move(renderData), NScenarios::TShowViewDirective_EInactivityTimeout_Medium);
    bodyBuilder.AddClientActionDirective("tts_play_placeholder", {});
}

void AddFrontalLedImage(const TVector<TString>& imageUrls, TResponseBodyBuilder* responseBodyBuilder) {
    responseBodyBuilder->AddClientActionDirective(TString{FORCE_DISPLAY_CARDS_DIRECTIVE_NAME}, {});
    NJson::TJsonValue payload;
    for (const auto& imageUrl : imageUrls) {
        NJson::TJsonValue value;
        value["frontal_led_image"] = imageUrl;
        payload["payload"].AppendValue(std::move(value));
    }
    responseBodyBuilder->AddClientActionDirective(TString{FRONTAL_LED_IMAGE_DIRECTIVE_NAME}, payload);
}

void AddScledEyes(TResponseBodyBuilder* responseBodyBuilder) {
    TScledAnimationBuilder scled;
    constexpr TScledAnimationBuilder::TPattern eye = TScledAnimationBuilder::MakeDigit(true, true, false, false, false, true, false);
    TScledAnimationBuilder::TFullPattern pattern = TScledAnimationBuilder::MakeFullPattern(eye, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), eye);
    scled.AddDraw(pattern, /* brighness= */ 255, /* durationMs= */ 2000);
    NScledAnimation::AddDrawScled(*responseBodyBuilder, scled);
    responseBodyBuilder->AddTtsPlayPlaceholderDirective();
}


void AddAction(const TString& frameName, TResponseBodyBuilder& bodyBuilder) {
    NScenarios::TFrameAction action;
    action.MutableNluHint()->SetFrameName(frameName);
    bodyBuilder.AddAction(frameName, std::move(action));
}


void AddPush(TResponseBodyBuilder& bodyBuilder, const TString& title, const TString& text,
             const TString& link, const TString& imageUrl, const TString& pushId)
{
    TPushDirectiveBuilder(title, text, link, pushId)
        .SetPushId(pushId)
        .SetCardImageUrl(imageUrl)
        .SetThrottlePolicy("eddl-unlimitted")
        .BuildTo(bodyBuilder);
}

void AddListenDirectiveWithCallback(int silenceTimeoutMs, TResponseBodyBuilder* bodyBuilder) {
    auto& directives = *bodyBuilder->GetResponseBody().MutableLayout()->MutableDirectives();

    bodyBuilder->AddListenDirective(silenceTimeoutMs);

    auto& callbackDirective = *directives.Add()->MutableCallbackDirective();
    callbackDirective.SetName(SILENCE_CALLBACK);
}

} // namespace NAlice::NHollywood::NGeneralConversation
