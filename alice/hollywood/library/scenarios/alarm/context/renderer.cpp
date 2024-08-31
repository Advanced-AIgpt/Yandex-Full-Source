#include "renderer.h"

#include <alice/hollywood/library/scenarios/alarm/util/util.h>

#include <alice/library/scled_animations/scled_animations_builder.h>
#include <alice/library/scled_animations/scled_animations_directive_hw.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/string/join.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NReminders {

namespace {

template<typename TSlotsIterable>
void FillSlotsData(TNlgData& nlgData, TSlotsIterable slots) {
    for (const auto& slot: slots) {
        NJson::TJsonValue slotJson;
        if (NJson::ReadJsonFastTree(slot.Value.AsString(), &slotJson)) {
            nlgData.Context[slot.Name] = slotJson;
        } else {
            nlgData.Context[slot.Name] = slot.Value.AsString();
        }
    }
}

} // namespace

TRenderer::TRenderer(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& runRequest)
    : NlgData_{ctx.Ctx.Logger(), runRequest}
    , NlgWrapper_{TNlgWrapper::Create(ctx.Ctx.Nlg(), runRequest, ctx.Rng, ctx.UserLang)}
    , Builder_{&NlgWrapper_}
    , BodyBuilder_{Builder_.CreateResponseBodyBuilder()}
    , AnalyticsInfoBuilder_{BodyBuilder_.CreateAnalyticsInfoBuilder()}
    , Buttons_{}
{
    NlgData_.Context["attentions"] = NJson::TJsonMap();

    for (const auto& expFlag : NExperiments::NLG_EXPERIMENTS) {
        if (runRequest.HasExpFlag(expFlag)) {
            NlgData_.Context[expFlag] = true;
        }
    }
}

TRunResponseBuilder& TRenderer::Builder() {
    return Builder_;
}

void TRenderer::SetProductScenarioName(const TString& productScenarioName) {
    AnalyticsInfoBuilder_.SetProductScenarioName(productScenarioName);
}

void TRenderer::SetIntentName(const TString& intentName) {
    IntentName_ = intentName;
    AnalyticsInfoBuilder_.SetIntentName(intentName);
    Builder_.GetMutableFeatures().SetIntent(intentName);
}

void TRenderer::SetIrrelevant() {
    Builder_.SetIrrelevant();
}

void TRenderer::SetNotSupported(const TStringBuf nlgTemplateName) {
    AddVoiceCard(
        nlgTemplateName,
        NNlgTemplateNames::NOT_SUPPORTED
    );
}

void TRenderer::SetShouldListen(bool shouldListen) {
    BodyBuilder_.SetShouldListen(shouldListen);
}

void TRenderer::AddAttention(const TStringBuf attention) {
    NlgData_.AddAttention(attention);
}

void TRenderer::AddAction(const TString& actionId, NScenarios::TFrameAction&& action) {
    BodyBuilder_.AddAction(actionId, std::move(action));
}

void TRenderer::AddDirective(NScenarios::TDirective&& directive) {
    BodyBuilder_.AddDirective(std::move(directive));
}

void TRenderer::AddVoiceCard(const TStringBuf nlgTemplateName, const TStringBuf phraseName, NJson::TJsonValue cardData) {
    VoiceCardRenderInfos_.emplace_back(nlgTemplateName, phraseName, std::move(cardData));
}

void TRenderer::AddError(const TStringBuf& errorCode) {
    NlgData_.Context["error"]["data"]["code"] = errorCode;
}

void TRenderer::AddTypeTextSuggest(const TStringBuf type, NJson::TJsonValue suggestsData, const TString& actionId) {
    NlgData_.Context["suggests_data"][type] = std::move(suggestsData);
    TString suggestUtterance = NlgWrapper_.RenderPhrase(NNlgTemplateNames::SUGGESTS, TString::Join("render_suggest_utterance__", type), NlgData_).Text;
    TString suggestCaption = NlgWrapper_.RenderPhrase(NNlgTemplateNames::SUGGESTS, TString::Join("render_suggest_caption__", type), NlgData_).Text;

    TFrameAction action;

    TDirective directive;
    TTypeTextDirective* typeTextDirective = directive.MutableTypeTextDirective();
    typeTextDirective->SetText(suggestUtterance);
    typeTextDirective->SetName("type");
    *action.MutableDirectives()->AddList() = directive;

    BodyBuilder_.AddAction(actionId, std::move(action));
    BodyBuilder_.AddActionSuggest(actionId).Title(suggestCaption);
}

void TRenderer::AddOpenUriSuggest(const TStringBuf type, NJson::TJsonValue suggestsData, const TString& actionId, bool addButton) {
    NlgData_.Context["suggests_data"][type] = std::move(suggestsData);
    TString suggestUri = NlgWrapper_.RenderPhrase(NNlgTemplateNames::SUGGESTS, TString::Join("render_suggest_uri__", type), NlgData_).Text;
    TString suggestCaption = NlgWrapper_.RenderPhrase(NNlgTemplateNames::SUGGESTS, TString::Join("render_suggest_caption__", type), NlgData_).Text;

    TFrameAction action;

    TDirective directive;
    TOpenUriDirective* openUriDirective = directive.MutableOpenUriDirective();
    openUriDirective->SetUri(suggestUri);
    openUriDirective->SetName("open_uri");
    *action.MutableDirectives()->AddList() = directive;

    BodyBuilder_.AddAction(actionId, std::move(action));
    BodyBuilder_.AddActionSuggest(actionId).Title(suggestCaption);

    if (addButton) {
        NScenarios::TLayout::TButton button;
        button.SetTitle(suggestCaption);
        button.SetActionId(actionId);

        Buttons_.emplace_back(button);
    }

}

void TRenderer::AddDirectiveSuggest(const TStringBuf type, NJson::TJsonValue suggestsData, const TString& actionId, const NScenarios::TDirective& suggestDirective, bool addButton) {
    NlgData_.Context["suggests_data"][type] = std::move(suggestsData);
    TString suggestCaption = NlgWrapper_.RenderPhrase(NNlgTemplateNames::SUGGESTS, TString::Join("render_suggest_caption__", type), NlgData_).Text;

    TFrameAction action;

    *action.MutableDirectives()->AddList() = suggestDirective;

    BodyBuilder_.AddAction(actionId, std::move(action));
    BodyBuilder_.AddActionSuggest(actionId).Title(suggestCaption);

    if (addButton) {
        NScenarios::TLayout::TButton button;
        button.SetTitle(suggestCaption);
        button.SetActionId(actionId);

        Buttons_.emplace_back(button);
    }

}

void TRenderer::AddScledAnimationAlarmTimeDirective(const NDatetime::TCivilSecond& time, const NDatetime::TCivilSecond& /* currTime */) {
    const auto pattern = ConstructedScledPattern(time, "%02u:%02u ");
    AddScledAnimationTimeDirective(pattern);
}

void TRenderer::AddScledAnimationTimerTimeDirective(const TDuration& time) {
    const auto pattern = Sprintf("%02lu:%02lu ", time.Hours(), time.Minutes() % 60);
    AddScledAnimationTimeDirective(pattern);
}

void TRenderer::AddScledAnimationTimeDirective(const TString& pattern) {
    TScledAnimationBuilder animBuilder;
    animBuilder.AddAnim(
        pattern,
        /* bright1= */ 0,
        /* bright2= */ 255,
        /* durationMs= */ 1000,
        TScledAnimationBuilder::AnimModeFromRight | TScledAnimationBuilder::AnimModeSpeedSmooth
    );

    animBuilder.AddDraw(pattern, /* brightness= */ 255, /* durationMs= */ 1000);

    animBuilder.AddAnim(
        pattern,
        /* bright1= */ 255,
        /* bright2= */ 0,
        /* durationMs= */ 1000,
        TScledAnimationBuilder::AnimModeFromLeft | TScledAnimationBuilder::AnimModeSpeedSmooth
    );

    animBuilder.SetAnim("     *", /* bright1= */ 0, /* bright2= */ 255, /* from= */ 2880, /* to= */ 3240, TScledAnimationBuilder::AnimModeFade);
    animBuilder.SetAnim("     *", /* bright1= */ 255, /* bright2= */ 0, /* from= */ 3240, /* to= */ 3560, TScledAnimationBuilder::AnimModeFade);

    animBuilder.SetAnim("     *", /* bright1= */ 0, /* bright2= */ 255, /* from= */ 3540, /* to= */ 3920, TScledAnimationBuilder::AnimModeFade);
    animBuilder.SetAnim("     *", /* bright1= */ 255, /* bright2= */ 0, /* from= */ 3920, /* to= */ 4280, TScledAnimationBuilder::AnimModeFade);

    NScledAnimation::TScledAnimationOptions options;
    options.AddSpeakingAnimationFlag = false;
    NScledAnimation::AddDrawScled(BodyBuilder_, animBuilder, options);
}

void TRenderer::AddFrameSlots(const TFrame& frame) {
    FillSlotsData(NlgData_, frame.Slots());
}

TResetAddBuilder TRenderer::ResetAddBuilder() {
    return BodyBuilder_.ResetAddBuilder();
}

void TRenderer::AddTtsPlayPlaceholderDirective() {
    return BodyBuilder_.AddTtsPlayPlaceholderDirective();
}

bool TRenderer::TryAddShowPromoDirective(const NAlice::NScenarios::TInterfaces& interfaces) {
    if (interfaces.GetSupportsShowPromo()) {
        BodyBuilder_.AddShowPromoDirective();
        return true;
    }
    return false;
}

void TRenderer::Render() {
    NlgData_.Context["intent_name"] = IntentName_;

    if (IntentName_ == NFrameNames::ALARM_WHAT_SOUND_IS_SET) {
        NlgData_.Context["is_alarm_what_sound_is_set_intent"] = true;
    }

    if (IntentName_ == NFrameNames::ALARM_SET_SOUND || IntentName_ == NFrameNames::ALARM_ASK_SOUND) {
        NlgData_.Context["is_alarm_set_sound_intent"] = true;
    }

    for (auto& cardRenderInfo: VoiceCardRenderInfos_) {
        NlgData_.Context["data"] = std::move(cardRenderInfo.CardData);

        BodyBuilder_.AddRenderedTextWithButtonsAndVoice(
            cardRenderInfo.NlgTemplateName,
            cardRenderInfo.PhraseName,
            Buttons_,
            NlgData_
        );
    }
}

} // namespace NAlice::NHollywood::NWeather
