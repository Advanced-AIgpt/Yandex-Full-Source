#pragma once

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/hollywood/library/scenarios/weather/background_sounds/background_sounds.h>
#include <alice/hollywood/library/scenarios/weather/util/condition_provider.h>
#include <alice/hollywood/library/scenarios/weather/util/error.h>

#include <alice/hollywood/library/capability_wrapper/capability_wrapper.h>

namespace NAlice::NHollywood::NWeather {

enum class ESuggestType {
    Today,
    Tomorrow,
    AfterTomorrow,
    Weekend,
    SearchFallback,
    Onboarding,
    NowcastWhenEnds,
    NowcastWhenStarts,
    Feedback,
    OpenUri,
};

class TRenderer {
private:
    using TButton = NScenarios::TLayout::TButton;

public:
    TRenderer(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& runRequest, const TFrame* frame = nullptr);

    TRunResponseBuilder& Builder();
    TBackgroundSounds& BackgroundSounds();

    void SetProductScenarioName(const TString& productScenarioName);
    void SetIntentName(const TString& intentName);
    void AddAttention(const TStringBuf attention);

    void AddError(const EWeatherErrorCode code);
    void AddDivCard(const TStringBuf nlgTemplateName, const TStringBuf cardName, NJson::TJsonValue cardData = NJson::TJsonValue());
    void AddTextCard(const TStringBuf nlgTemplateName, const TStringBuf phraseName, NJson::TJsonValue cardData = NJson::TJsonValue());
    void AddTextWithButtons(const TStringBuf nlgTemplateName, const TStringBuf phraseName,
                            const TVector<TButton>& buttons, NJson::TJsonValue cardData = NJson::TJsonValue());
    void AddVoiceCard(const TStringBuf nlgTemplateName, const TStringBuf phraseName, NJson::TJsonValue cardData = NJson::TJsonValue());
    void AddAnimationDirectives(const IConditionProvider& conditionProvider);
    void AddOpenUriDirective(const TStringBuf uri);
    void AddSuggests(TVector<ESuggestType> suggests);

    TButton CreateOpenUriButton(const TString& buttonType, const TString& buttonUri);

    void RenderFeedbackAnswer(const NScenarios::TCallbackDirective* callback);
    void Render();

private:
    using TCardRenderInfo = std::tuple<
        TString, // nlgTemplateName
        TString, // cardName or phraseName
        TVector<TButton>, // buttons for https://st.yandex-team.ru/WEATHER-18060
        NJson::TJsonValue // cardData
    >;

private:
    void AddLedScreenDirective(const TVector<TString>& imageUris);
    void AddScledAnimationDirective(const int temperature);

    void AddSpecialSuggestWithAction(const TStringBuf name, const TStringBuf type);
    void AddTypeTextSuggest(const TStringBuf type);
    void AddOpenUriSuggest(const TStringBuf type);
    void AddSearchSuggest(const TStringBuf query);
    void AddFeedbackSuggests();
    void RenderSuggests();

private:
    TScenarioHandleContext& Ctx_;
    const TFrame* Frame_;
    const TScenarioRunRequestWrapper& RunRequest_;
    const TCapabilityWrapper<TScenarioRunRequestWrapper> CapabilityWrapper_;

    TNlgData NlgData_;
    TNlgWrapper NlgWrapper_;
    TRunResponseBuilder Builder_;
    TResponseBodyBuilder& BodyBuilder_;
    NScenarios::IAnalyticsInfoBuilder& AnalyticsInfoBuilder_;
    TString IntentName_;

    TBackgroundSounds BackgroundSounds_;

    TVector<ESuggestType> Suggests_;
    TVector<TCardRenderInfo> DivCardRenderInfos_;
    TVector<TCardRenderInfo> TextCardRenderInfos_;
    TVector<TCardRenderInfo> VoiceCardRenderInfos_;
    bool IsFeedbackInput_;
};

} // namespace NAlice::NHollywood::NWeather
