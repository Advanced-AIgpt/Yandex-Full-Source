#pragma once

#include <alice/hollywood/library/scenarios/time_capsule/proto/time_capsule.pb.h>

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/base_scenario/scenario.h>

#include <library/cpp/timezone_conversion/civil.h>

namespace NAlice::NHollywood::NTimeCapsule {

class TScene {
public:
    TScene(const TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& runRequest);

    void SetProductScenarioName(const TString& productScenarioName);
    void SetIntentName(const TString& intentName);
    void AddAnalyticsObject(const TString& objectName, const TString& objectValue);

    void SetIrrelevant();
    void SetNotSupported(const TStringBuf nlgTemplateName);

    void SetExpectsRequest(bool expectsRequest);
    void SetShouldListen(bool shouldListen);

    void AddAttention(const TStringBuf attention);
    void AddAction(const TString& actionId, NScenarios::TFrameAction&& action);
    void AddServerDirective(NScenarios::TServerDirective&& directive);
    void AddDirective(NScenarios::TDirective&& directive);
    void AddError(const TStringBuf& errorCode);
    void AddTtsPlayPlaceholderDirective();

    void AddVoiceCard(const TStringBuf nlgTemplateName, const TStringBuf phraseName, NJson::TJsonValue cardData = NJson::TJsonValue());
    void AddDiv2Card(const TStringBuf nlgTemplateName, const TStringBuf cardName, NJson::TJsonValue cardData = NJson::TJsonValue());
    void AddTypeTextSuggest(const TStringBuf type, NJson::TJsonValue suggestsData, const TString& actionId);

    void SetState(const TTimeCapsuleState& timeCapsuleState);

    void RenderTo(TScenarioHandleContext& ctx);

private:
    struct TCardRenderInfo {
        TCardRenderInfo(
            const TStringBuf nlgTemplateName,
            const TStringBuf phraseName,
            const TStringBuf cardName,
            NJson::TJsonValue cardData
        )
            : NlgTemplateName(nlgTemplateName)
            , PhraseName(phraseName)
            , CardName(cardName)
            , CardData(cardData)
        {
        }

        TString  NlgTemplateName;
        TString  PhraseName;
        TString  CardName;
        NJson::TJsonValue CardData;
    };

private:
    TNlgData NlgData_;
    TNlgWrapper NlgWrapper_;
    TRunResponseBuilder Builder_;
    TResponseBodyBuilder& BodyBuilder_;
    NScenarios::IAnalyticsInfoBuilder& AnalyticsInfoBuilder_;
    TString IntentName_;

    TVector<TCardRenderInfo> VoiceCardRenderInfos_;
    TVector<TCardRenderInfo> Div2CardRenderInfos_;
    TVector<NScenarios::TLayout::TButton> Buttons_;
};

} // namespace NAlice::NHollywood::NReminders
