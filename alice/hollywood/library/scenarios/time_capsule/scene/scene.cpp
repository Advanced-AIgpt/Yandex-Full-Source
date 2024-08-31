#include "scene.h"

#include <alice/hollywood/library/scenarios/time_capsule/util/util.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/string/join.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NTimeCapsule {

namespace {

constexpr TStringBuf NOT_SUPPORTED  = "not_supported";

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

TScene::TScene(const TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& runRequest)
    : NlgData_{ctx.Ctx.Logger(), runRequest}
    , NlgWrapper_(TNlgWrapper::Create(ctx.Ctx.Nlg(), runRequest, ctx.Rng, ctx.UserLang))
    , Builder_{&NlgWrapper_}
    , BodyBuilder_{Builder_.CreateResponseBodyBuilder()}
    , AnalyticsInfoBuilder_{BodyBuilder_.CreateAnalyticsInfoBuilder()}
    , Buttons_{}
{
    NlgData_.Context["attentions"] = NJson::TJsonMap();
}

void TScene::SetProductScenarioName(const TString& productScenarioName) {
    AnalyticsInfoBuilder_.SetProductScenarioName(productScenarioName);
}

void TScene::SetIntentName(const TString& intentName) {
    IntentName_ = intentName;
    AnalyticsInfoBuilder_.SetIntentName(intentName);
    Builder_.GetMutableFeatures().SetIntent(intentName);
}

void TScene::AddAnalyticsObject(const TString& objectName, const TString& objectValue) {
    AnalyticsInfoBuilder_.AddObject(objectName, objectName, objectValue);
}

void TScene::SetIrrelevant() {
    Builder_.SetIrrelevant();
}

void TScene::SetNotSupported(const TStringBuf nlgTemplateName) {
    AddVoiceCard(
        nlgTemplateName,
        NOT_SUPPORTED
    );
}

void TScene::SetExpectsRequest(bool expectsRequest) {
    BodyBuilder_.SetExpectsRequest(expectsRequest);
}

void TScene::SetShouldListen(bool shouldListen) {
    BodyBuilder_.SetShouldListen(shouldListen);
}

void TScene::AddAttention(const TStringBuf attention) {
    NlgData_.AddAttention(attention);
}

void TScene::AddAction(const TString& actionId, NScenarios::TFrameAction&& action) {
    BodyBuilder_.AddAction(actionId, std::move(action));
}

void TScene::AddServerDirective(NScenarios::TServerDirective&& serverDirective) {
    BodyBuilder_.AddServerDirective(std::move(serverDirective));
}

void TScene::AddDirective(NScenarios::TDirective&& directive) {
    BodyBuilder_.AddDirective(std::move(directive));
}

void TScene::AddVoiceCard(const TStringBuf nlgTemplateName, const TStringBuf phraseName, NJson::TJsonValue cardData) {
    VoiceCardRenderInfos_.emplace_back(nlgTemplateName, phraseName, /* cardName = */ "", std::move(cardData));
}

void TScene::AddDiv2Card(const TStringBuf nlgTemplateName, const TStringBuf cardName, NJson::TJsonValue cardData) {
    Div2CardRenderInfos_.emplace_back(nlgTemplateName, /* phraseName = */ "", cardName, std::move(cardData));
}

void TScene::AddTypeTextSuggest(const TStringBuf type, NJson::TJsonValue suggestsData, const TString& actionId) {
    NlgData_.Context["suggests_data"][type] = std::move(suggestsData);
    const TString suggestUtterance = NlgWrapper_.RenderPhrase(NNlgTemplateNames::SUGGESTS, TString::Join("render_suggest_utterance__", type), NlgData_).Text;
    const TString suggestCaption = NlgWrapper_.RenderPhrase(NNlgTemplateNames::SUGGESTS, TString::Join("render_suggest_caption__", type), NlgData_).Text;

    TFrameAction action;

    TDirective directive;
    TTypeTextDirective* typeTextDirective = directive.MutableTypeTextDirective();
    typeTextDirective->SetText(suggestUtterance);
    typeTextDirective->SetName("type");
    *action.MutableDirectives()->AddList() = directive;

    BodyBuilder_.AddAction(actionId, std::move(action));
    BodyBuilder_.AddActionSuggest(actionId).Title(suggestCaption);
}

void TScene::AddError(const TStringBuf& errorCode) {
    NlgData_.Context["error"]["data"]["code"] = errorCode;
}

void TScene::AddTtsPlayPlaceholderDirective() {
    BodyBuilder_.AddTtsPlayPlaceholderDirective();
}

void TScene::SetState(const TTimeCapsuleState& timeCapsuleState) {
    BodyBuilder_.SetState(timeCapsuleState);
}

void TScene::RenderTo(TScenarioHandleContext& ctx) {
    NlgData_.Context["intent_name"] = IntentName_;

    for (auto& cardRenderInfo : Div2CardRenderInfos_) {
        NlgData_.Context["data"] = std::move(cardRenderInfo.CardData);
        BodyBuilder_.AddRenderedDiv2Card(
            cardRenderInfo.NlgTemplateName,
            cardRenderInfo.CardName,
            NlgData_
        );
    }

    for (auto& cardRenderInfo : VoiceCardRenderInfos_) {
        NlgData_.Context["data"] = std::move(cardRenderInfo.CardData);

        BodyBuilder_.AddRenderedTextWithButtonsAndVoice(
            cardRenderInfo.NlgTemplateName,
            cardRenderInfo.PhraseName,
            Buttons_,
            NlgData_
        );
    }

    ctx.ServiceCtx.AddProtobufItem(*std::move(Builder_).BuildResponse(), RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NTimeCapsule
