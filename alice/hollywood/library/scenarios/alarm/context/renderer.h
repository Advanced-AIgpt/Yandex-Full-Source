#pragma once

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/base_scenario/scenario.h>

#include <library/cpp/timezone_conversion/civil.h>

namespace NAlice::NHollywood::NReminders {

class TRenderer {
public:
    TRenderer(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& runRequest);

    TRunResponseBuilder& Builder();

    void SetProductScenarioName(const TString& productScenarioName);
    void SetIntentName(const TString& intentName);
    void SetIrrelevant();
    void SetNotSupported(const TStringBuf nlgTemplateName);
    void SetShouldListen(bool shouldListen);
    void AddAttention(const TStringBuf attention);
    void AddAction(const TString& actionId, NScenarios::TFrameAction&& action);
    void AddDirective(NScenarios::TDirective&& directive);
    void AddError(const TStringBuf& errorCode);

    void AddTypeTextSuggest(const TStringBuf type, NJson::TJsonValue suggestsData, const TString& actionId);
    void AddOpenUriSuggest(const TStringBuf type, NJson::TJsonValue suggestsData, const TString& actionId, bool addButton);
    void AddDirectiveSuggest(const TStringBuf type, NJson::TJsonValue suggestsData, const TString& actionId, const NScenarios::TDirective& suggestDirective, bool addButton);

    void AddScledAnimationAlarmTimeDirective(const NDatetime::TCivilSecond& time, const NDatetime::TCivilSecond& currTime);
    void AddScledAnimationTimerTimeDirective(const TDuration& time);

    void AddFrameSlots(const TFrame& frame);
    void AddVoiceCard(const TStringBuf nlgTemplateName, const TStringBuf phraseName, NJson::TJsonValue cardData = NJson::TJsonValue());
    void AddTtsPlayPlaceholderDirective();

    bool TryAddShowPromoDirective(const NAlice::NScenarios::TInterfaces& interfaces);

    TResetAddBuilder ResetAddBuilder();

    void Render();

private:
    struct TCardRenderInfo {
        TCardRenderInfo(
            const TStringBuf nlgTemplateName,
            const TStringBuf phraseName,
            NJson::TJsonValue cardData
        )
            : NlgTemplateName(nlgTemplateName)
            , PhraseName(phraseName)
            , CardData(cardData)
        {
        }

        TString  NlgTemplateName;
        TString  PhraseName;
        NJson::TJsonValue CardData;
    };

    void AddScledAnimationTimeDirective(const TString& time);

private:
    TNlgData NlgData_;
    TNlgWrapper NlgWrapper_;
    TRunResponseBuilder Builder_;
    TResponseBodyBuilder& BodyBuilder_;
    NScenarios::IAnalyticsInfoBuilder& AnalyticsInfoBuilder_;
    TString IntentName_;

    TVector<TCardRenderInfo> VoiceCardRenderInfos_;
    TVector<NScenarios::TLayout::TButton> Buttons_;
};

} // namespace NAlice::NHollywood::NReminders
