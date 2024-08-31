#include "renderer.h"

#include <alice/hollywood/library/scenarios/weather/s3_animations/s3_animations.h>
#include <alice/hollywood/library/scenarios/weather/util/error.h>

#include <alice/hollywood/library/scenarios/weather/util/util.h>
#include <alice/library/experiments/experiments.h>

#include <alice/library/scled_animations/scled_animations_builder.h>
#include <alice/library/scled_animations/scled_animations_directive_hw.h>
#include <alice/library/versioning/versioning.h>

#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/endpoint/capability.pb.h>

#include <util/string/join.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NWeather {

namespace {

constexpr TStringBuf EXP_MOCK_TIME_PREFIX = "mock_time=";

constexpr TStringBuf FEEDBACK_PRODUCT_SCENARIO_NAME = "feedback";

constexpr TStringBuf POSITIVE_FEEDBACK_INTENT = "personal_assistant.feedback.feedback_positive";
constexpr TStringBuf NEGATIVE_FEEDBACK_INTENT = "personal_assistant.feedback.feedback_negative";
constexpr TStringBuf CONTINUE_FEEDBACK_INTENT = "personal_assistant.feedback.feedback_continue";

constexpr TStringBuf SCREEN_ID_CLOUD_UI = "cloud_ui";

constexpr TStringBuf WEATHER = "weather";
constexpr TStringBuf WEATHER_GIF_VERSION = "1";
constexpr TStringBuf WEATHER_GIF_DEFAULT_SUBVERSION = "4";

enum EPosition { Begin, End };

void MoveElement(const ESuggestType element, TVector<ESuggestType>& suggests, const EPosition position,
                 const bool skipNonExisting = false) {
    const auto it = Find(suggests, element);
    const bool exist = it != suggests.end();
    if (exist) {
        suggests.erase(it);
    }
    if (exist || !skipNonExisting) {
        suggests.insert(position == EPosition::Begin ? suggests.begin() : suggests.end(), element);
    }
}

void AlterSuggestsForExperiments(const TScenarioRunRequestWrapper& runRequest, TVector<ESuggestType>& suggests) {
    // See https://st.yandex-team.ru/WEATHER-15572#5e2012e0c87811669e29999f
    const bool isNewSuggests =
        runRequest.HasExpFlag(NExperiment::WEATHER_SUGGEST_3) || runRequest.HasExpFlag(NExperiment::WEATHER_SUGGEST_4);
    const bool shouldAddNowcastSuggest =
        runRequest.HasExpFlag(NExperiment::WEATHER_SUGGEST_2) || runRequest.HasExpFlag(NExperiment::WEATHER_SUGGEST_7);
    // We suggest that the suggests come in the new form. We remove all the new elements from them if the exps require
    // the old form
    if (!isNewSuggests && IsIn(suggests, ESuggestType::Tomorrow)) {
        if (const auto it = Find(suggests, ESuggestType::AfterTomorrow); it != suggests.end()) {
            suggests.erase(it);
        }
    }
    if (!isNewSuggests && !shouldAddNowcastSuggest) {
        if (const auto it = Find(suggests, ESuggestType::NowcastWhenStarts); it != suggests.end()) {
            suggests.erase(it);
        }
        if (const auto it = Find(suggests, ESuggestType::NowcastWhenEnds); it != suggests.end()) {
            suggests.erase(it);
        }
    }

    // It catches the tricky "погода на следующие выходные" case for removing the search suggest
    // TODO(sparkle): ask stansidel@ for this case
    //const bool isNextWeekend = IsIn(suggests, ESuggestType::Today) && IsIn(suggests, ESuggestType::Tomorrow);
    //if (!isNewSuggests && isNextWeekend) {
        //if (const auto it = Find(suggests, ESuggestType::SearchFallback); it != suggests.end()) {
            //suggests.erase(it);
        //}
    //}

    // Alter the suggests for each experiment
    if (runRequest.HasExpFlag(NExperiment::WEATHER_SUGGEST_1)) {
        MoveElement(ESuggestType::Onboarding, suggests, EPosition::Begin);
        return;
    }
    if (runRequest.HasExpFlag(NExperiment::WEATHER_SUGGEST_2)) {
        // Do nothing, already preserved the new suggest above (`shouldAddNowcastSuggest`)
        return;
    }
    if (runRequest.HasExpFlag(NExperiment::WEATHER_SUGGEST_3)) {
        MoveElement(ESuggestType::SearchFallback, suggests, EPosition::End);
        return;
    }
    if (runRequest.HasExpFlag(NExperiment::WEATHER_SUGGEST_4)) {
        MoveElement(ESuggestType::Feedback, suggests, EPosition::End);
        return;
    }
    if (runRequest.HasExpFlag(NExperiment::WEATHER_SUGGEST_5)) {
        MoveElement(ESuggestType::Feedback, suggests, EPosition::End);
        MoveElement(ESuggestType::SearchFallback, suggests, EPosition::Begin);
        return;
    }
}

TMaybe<i64> TryFindMockTimestamp(const TScenarioRunRequestWrapper& runRequest) {
    if (const auto exp = GetExperimentValueWithPrefix(runRequest.ExpFlags(), EXP_MOCK_TIME_PREFIX)) {
        return TInstant::ParseIso8601(*exp).MicroSeconds();
    }
    return Nothing();
}

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

void FillCallbackDirective(TCallbackDirective& callbackDirective, const TStringBuf name, const TStringBuf type) {
    callbackDirective.SetName(name.data(), name.size());
    auto& payloadFields = *callbackDirective.MutablePayload()->mutable_fields();

    ::google::protobuf::Value suggestTypePB;
    suggestTypePB.set_string_value(type.data(), type.size());
    payloadFields["type"] = std::move(suggestTypePB);
}

TString GetPrecipationGifName(const int precType, const double precStrength) {
    if (precType == 1) {  // rain
        if (precStrength <= 0.25) {
            return "rain_low";
        } else if (precStrength >= 0.75) {
            return "rain_hi";
        } else {
            return "rain_medium";
        }
    } else if (precType == 3) {  // snow
        if (precStrength <= 0.25) {
            return "snow_low";
        } else if (precStrength >= 0.75) {
            return "snow_hi";
        } else {
            return "snow_medium";
        }
    } else if (precType == 2) {  // sleet
        return "snow_rain";
    }
    return "";
}

TVector<TString> MakeWeatherGifUris(const TExpFlags& expFlags, const IConditionProvider& conditionProvider) {
    TVector<TString> gifNames({
        TString::Join("temperature/", ToString(conditionProvider.GetTemp())),
    });

    const auto precType = conditionProvider.GetPrecType();
    const auto precStrength = conditionProvider.GetPrecStrength();
    const auto cloudness = conditionProvider.GetCloudness();

    if (cloudness < 0.01 && precStrength < 0.01) {  // clear
        gifNames.push_back("sun");
    } else if (precStrength < 0.01) {  // cloudy
        gifNames.push_back("clouds");
    } else if (precType > 0) {  // precipitation
        TString precipitationGifName = GetPrecipationGifName(precType, precStrength);
        if (precipitationGifName) {
            gifNames.push_back(precipitationGifName);
        }
    }

    const auto subversion = GetGifVersion(expFlags, WEATHER, WEATHER_GIF_DEFAULT_SUBVERSION);
    TVector<TString> gifUris;
    for (const auto& gifName : gifNames) {
        gifUris.push_back(
            FormatGifVersion(
                WEATHER,
                TString::Join(gifName, ".gif"),
                WEATHER_GIF_VERSION,
                subversion
            )
        );
    }

    return gifUris;
}

} // namespace

TRenderer::TRenderer(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& runRequest, const TFrame* frame)
    : Ctx_{ctx}
    , Frame_{frame}
    , RunRequest_{runRequest}
    , CapabilityWrapper_{RunRequest_, GetEnvironmentStateProto(RunRequest_)}
    , NlgData_{ctx.Ctx.Logger(), runRequest}
    , NlgWrapper_(TNlgWrapper::Create(ctx.Ctx.Nlg(), RunRequest_, ctx.Rng, ctx.UserLang))
    , Builder_{&NlgWrapper_}
    , BodyBuilder_{Builder_.CreateResponseBodyBuilder(frame)}
    , AnalyticsInfoBuilder_{BodyBuilder_.CreateAnalyticsInfoBuilder()}
    , BackgroundSounds_{ctx.Rng}
    , IsFeedbackInput_{false}
{
    NlgData_.Context["attentions"] = NJson::TJsonMap();
    if (const auto timestamp = TryFindMockTimestamp(RunRequest_)) {
        NlgData_.Context["mock_timestamp"] = *timestamp;
    }
}

TRunResponseBuilder& TRenderer::Builder() {
    return Builder_;
}

TBackgroundSounds& TRenderer::BackgroundSounds() {
    return BackgroundSounds_;
}

void TRenderer::SetProductScenarioName(const TString& productScenarioName) {
    AnalyticsInfoBuilder_.SetProductScenarioName(productScenarioName);
}

void TRenderer::SetIntentName(const TString& intentName) {
    IntentName_ = intentName;
    AnalyticsInfoBuilder_.SetIntentName(intentName);
    Builder_.GetMutableFeatures().SetIntent(intentName);
}

void TRenderer::AddAttention(const TStringBuf attention) {
    NlgData_.AddAttention(attention);
}

void TRenderer::AddError(const EWeatherErrorCode code) {
    TString phraseName = TString::Join("render_weather_error__", ToString(code));
    AddTextCard(NNlgTemplateNames::ERRORS, phraseName);
}

void TRenderer::AddDivCard(const TStringBuf nlgTemplateName, const TStringBuf cardName, NJson::TJsonValue cardData) {
    DivCardRenderInfos_.emplace_back(nlgTemplateName, cardName, NoButtons(), std::move(cardData));
}

void TRenderer::AddTextCard(const TStringBuf nlgTemplateName, const TStringBuf phraseName, NJson::TJsonValue cardData) {
    TextCardRenderInfos_.emplace_back(nlgTemplateName, phraseName, NoButtons(), std::move(cardData));
}

void TRenderer::AddTextWithButtons(const TStringBuf nlgTemplateName, const TStringBuf phraseName,
                                   const TVector<TRenderer::TButton>& buttons, NJson::TJsonValue cardData) {
    TextCardRenderInfos_.emplace_back(nlgTemplateName, phraseName, buttons, std::move(cardData));
}

void TRenderer::AddVoiceCard(const TStringBuf nlgTemplateName, const TStringBuf phraseName, NJson::TJsonValue cardData) {
    VoiceCardRenderInfos_.emplace_back(nlgTemplateName, phraseName, NoButtons(), std::move(cardData));
}

void TRenderer::AddAnimationDirectives(const IConditionProvider& conditionProvider) {
    bool needAddTtsPlayPlaceholder = false;
    if (CapabilityWrapper_.SupportsS3Animations()) {
        const TMaybe<TStringBuf> animationPath = NS3Animations::TryGetS3AnimationPathFromCondition(conditionProvider.GetCondition());
        if (animationPath.Defined()) {
            constexpr auto speakingPolicy = TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy_PlaySpeakingEndOfTts;
            BodyBuilder_.AddDirective(BuildDrawAnimationDirective(*animationPath, speakingPolicy));
            needAddTtsPlayPlaceholder = true;
        }
    }
    if (CapabilityWrapper_.HasLedDisplay()) {
        AddLedScreenDirective(MakeWeatherGifUris(RunRequest_.ExpFlags(), conditionProvider));
        needAddTtsPlayPlaceholder = true;
    }
    if (CapabilityWrapper_.HasScledDisplay()) {
        AddScledAnimationDirective(conditionProvider.GetTemp());
        needAddTtsPlayPlaceholder = true;
    }
    if (needAddTtsPlayPlaceholder) {
        BodyBuilder_.AddTtsPlayPlaceholderDirective();
    }
}

void TRenderer::AddLedScreenDirective(const TVector<TString>& imageUris) {
    {
        NScenarios::TDirective directive;
        auto& drawLedScreenDirective = *directive.MutableDrawLedScreenDirective();
        for (const auto& imageUri : imageUris) {
            auto& drawItem = *drawLedScreenDirective.AddDrawItem();
            drawItem.SetFrontalLedImage(imageUri);
        }
        BodyBuilder_.AddDirective(std::move(directive));
    }
}

void TRenderer::AddScledAnimationDirective(const int temperature) {
    const TString pattern = ConstructScledPattern(temperature);

    TScledAnimationBuilder animBuilder;
    animBuilder.AddAnim(pattern, /* bright1= */ 0, /* bright2= */ 255, /* durationMs= */ 1000, TScledAnimationBuilder::AnimModeFromRight);
    animBuilder.AddDraw(pattern, /* brightness= */ 255, /* durationMs= */ 2000);
    animBuilder.AddAnim(pattern, /* bright1= */ 255, /* bright2= */ 0, /* durationMs= */ 1000, TScledAnimationBuilder::AnimModeFromLeft);
    NScledAnimation::AddDrawScled(BodyBuilder_, animBuilder);
}

void TRenderer::AddOpenUriDirective(const TStringBuf uri) {
    TDirective directive;
    TOpenUriDirective& openUriDirective = *directive.MutableOpenUriDirective();
    openUriDirective.SetUri(TString{uri});

    BodyBuilder_.AddDirective(std::move(directive));
}

void TRenderer::AddSuggests(TVector<ESuggestType> suggests) {
    Suggests_.insert(Suggests_.end(), suggests.begin(), suggests.end());
}

void TRenderer::RenderSuggests() {
    if (Suggests_.empty()) {
        return;
    }

    // For https://st.yandex-team.ru/WEATHER-15572
    AlterSuggestsForExperiments(RunRequest_, Suggests_);
    MoveElement(ESuggestType::Feedback, Suggests_, EPosition::Begin, /* skipNonExisting = */ true);

    for (ESuggestType st : Suggests_) {
        switch (st) {
            case ESuggestType::Today:
                AddTypeTextSuggest("forecast_today");
                break;
            case ESuggestType::Tomorrow:
                AddTypeTextSuggest("forecast_tomorrow");
                break;
            case ESuggestType::AfterTomorrow:
                AddTypeTextSuggest("forecast_aftertomorrow");
                break;
            case ESuggestType::Weekend:
                AddTypeTextSuggest("forecast_weekend");
                break;
            case ESuggestType::SearchFallback:
                AddSearchSuggest(RunRequest_.Input().Utterance());
                break;
            case ESuggestType::NowcastWhenStarts:
                AddTypeTextSuggest("nowcast_when_starts");
                break;
            case ESuggestType::NowcastWhenEnds:
                AddTypeTextSuggest("nowcast_when_ends");
                break;
            case ESuggestType::Onboarding:
                if (!RunRequest_.ClientInfo().IsYaAuto()) {
                    AddTypeTextSuggest("onboarding__what_can_you_do");
                }
                break;
            case ESuggestType::Feedback:
                AddFeedbackSuggests();
                break;
            case ESuggestType::OpenUri:
                if (!RunRequest_.ClientInfo().IsYaAuto()) {
                    AddOpenUriSuggest("weather__open_uri");
                }
                break;
        }
    }
}

void TRenderer::AddOpenUriSuggest(const TStringBuf type) {
    TString actionId = TString::Join("suggest_", type);

    TString suggestUri = NlgWrapper_.RenderPhrase(NNlgTemplateNames::SUGGESTS, TString::Join("render_suggest_uri__", type), NlgData_).Text;
    TString suggestCaption = NlgWrapper_.RenderPhrase(NNlgTemplateNames::SUGGESTS, TString::Join("render_suggest_caption__", type), NlgData_).Text;

    TFrameAction action;

    TDirective directive;
    TOpenUriDirective& openUriDirective = *directive.MutableOpenUriDirective();
    openUriDirective.SetUri(suggestUri);
    *action.MutableDirectives()->AddList() = std::move(directive);

    BodyBuilder_.AddAction(actionId, std::move(action));
    BodyBuilder_.AddActionSuggest(actionId).Title(suggestCaption);
}

void TRenderer::AddTypeTextSuggest(const TStringBuf type) {
    TString actionId = TString::Join("suggest_", type);

    // TODO(alexanderplat): utterance inside directive should be localized too
    TString suggestUtterance = NlgWrapper_.RenderPhrase(NNlgTemplateNames::SUGGESTS, TString::Join("render_suggest_utterance__", type), NlgData_).Text;
    TString suggestCaption = NlgWrapper_.RenderPhrase(NNlgTemplateNames::SUGGESTS, TString::Join("render_suggest_caption__", type), NlgData_).Text;

    TFrameAction action;

    TDirective directive;
    TTypeTextDirective* typeTextDirective = directive.MutableTypeTextDirective();
    typeTextDirective->SetText(suggestUtterance);
    typeTextDirective->SetName("type");
    *action.MutableDirectives()->AddList() = std::move(directive);

    BodyBuilder_.AddAction(actionId, std::move(action));
    BodyBuilder_.AddActionSuggest(actionId).Title(suggestCaption);
}

void TRenderer::AddSearchSuggest(const TStringBuf query) {
    // In Weather only surfaces which have Div cards can have search suggests
    if (!RunRequest_.Proto().GetBaseRequest().GetInterfaces().GetCanRenderDivCards()) {
        return;
    }

    // Don't add search suggest when no direct user utterance
    const auto& input = RunRequest_.Input().Proto();
    if (input.HasText() && input.GetText().GetFromSuggest()) {
        return;
    }

    const TString queryStr = TString{query};
    BodyBuilder_.AddSearchSuggest().Title(queryStr).Query(queryStr);
}

void TRenderer::AddSpecialSuggestWithAction(const TStringBuf name, const TStringBuf type) {
    // add suggest
    const auto& suggestPhrase = NlgWrapper_.RenderPhrase(NNlgTemplateNames::FEEDBACK, TString::Join("render_suggest_", type), NlgData_).Text;
    TString actionId = TString::Join("suggest_", type);
    BodyBuilder_.AddActionSuggest(actionId).Title(suggestPhrase);

    // add action - type_text and callback directives
    TFrameAction action;
    auto& directives = *action.MutableDirectives();

    directives.AddList()->MutableTypeTextSilentDirective()->SetText(suggestPhrase);
    FillCallbackDirective(*directives.AddList()->MutableCallbackDirective(), name, type);

    BodyBuilder_.AddAction(std::move(actionId), std::move(action));
}

void TRenderer::RenderFeedbackAnswer(const TCallbackDirective* callback) {
    const auto& payloadFields = callback->GetPayload().fields();

    SetProductScenarioName(TString{FEEDBACK_PRODUCT_SCENARIO_NAME});

    if (payloadFields.count("type")) {
        const TString type = payloadFields.at("type").string_value();
        LOG_INFO(Ctx_.Ctx.Logger()) << "Feedback type: " << type.Quote();

        if (type == NFeedbackOptions::POSITIVE) {
            SetIntentName(TString{POSITIVE_FEEDBACK_INTENT});
        } else if (type == NFeedbackOptions::NEGATIVE) {
            SetIntentName(TString{NEGATIVE_FEEDBACK_INTENT});
        } else {
            SetIntentName(TString{CONTINUE_FEEDBACK_INTENT});
        }

        AddTextCard(NNlgTemplateNames::FEEDBACK, /* phraseName = */ TString::Join("render_", type));

        if (type == NFeedbackOptions::NEGATIVE) {
            static const TSmallVec<TStringBuf> negativeFeedbackSuggestButtons = {
                NFeedbackOptionsReasons::BAD_ANSWER,
                NFeedbackOptionsReasons::ASR_ERROR,
                NFeedbackOptionsReasons::TTS_ERROR,
                NFeedbackOptionsReasons::OFFENSIVE_ANSWER,
                NFeedbackOptionsReasons::OTHER,
                NFeedbackOptionsReasons::ALL_GOOD,
            };
            for (const TStringBuf negativeFeedbackSuggestButton : negativeFeedbackSuggestButtons) {
                AddSpecialSuggestWithAction(CALLBACK_FEEDBACK_NAME, negativeFeedbackSuggestButton);
            }
        }

        AddSuggests({ESuggestType::Onboarding});
        IsFeedbackInput_ = true;
    }
}

void TRenderer::AddFeedbackSuggests() {
    if (!RunRequest_.Interfaces().GetSupportsFeedback()) {
        return;
    }

    AddSpecialSuggestWithAction(CALLBACK_FEEDBACK_NAME, NFeedbackOptions::POSITIVE);
    AddSpecialSuggestWithAction(CALLBACK_FEEDBACK_NAME, NFeedbackOptions::NEGATIVE);
}

void TRenderer::Render() {
    NlgData_.Context["intent_name"] = IntentName_;

    if (Frame_) {
        FillSlotsData(NlgData_, Frame_->Slots());
    }

    RenderSuggests();

    if (RunRequest_.HasExpFlag(NExperiment::WEATHER_ENABLE_BACKGROUND_SOUNDS)) {
        if (TMaybe<TString> bgs = BackgroundSounds_.TryCalculateBackgroundFilename(); bgs.Defined()) {
            NlgData_.Context["background_sound_filename"] = std::move(*bgs);
        }
    }

    for (auto& cardRenderInfo: TextCardRenderInfos_) {
        NlgData_.Context["data"] = std::move(std::get<3>(cardRenderInfo));
        BodyBuilder_.AddRenderedTextWithButtonsAndVoice(
            /* nlgTemplateName = */ std::get<0>(cardRenderInfo),
            /* phraseName = */ std::get<1>(cardRenderInfo),
            /* buttons = */ std::get<2>(cardRenderInfo),
            NlgData_
        );
    }

    for (auto& cardRenderInfo: VoiceCardRenderInfos_) {
        NlgData_.Context["data"] = std::move(std::get<3>(cardRenderInfo));
        BodyBuilder_.AddRenderedVoice(
            /* nlgTemplateName = */ std::get<0>(cardRenderInfo),
            /* phraseName = */ std::get<1>(cardRenderInfo),
            NlgData_
        );
    }

    for (auto& cardRenderInfo: DivCardRenderInfos_) {
        NlgData_.Context["data"] = std::move(std::get<3>(cardRenderInfo));
        BodyBuilder_.AddRenderedDivCard(
            /* nlgTemplateName = */ std::get<0>(cardRenderInfo),
            /* cardName = */ std::get<1>(cardRenderInfo),
            NlgData_,
            /* reduceWhitespace = */ true
        );
    }

    if (auto forecastUri = NlgData_.Context["weather_forecast"]["uri"].GetString(); !forecastUri.empty() && CapabilityWrapper_.SupportsCloudUi() && CapabilityWrapper_.CanOpenLink()) {
        TDirective directive;
        directive.MutableOpenUriDirective()->SetScreenId(SCREEN_ID_CLOUD_UI.data(), SCREEN_ID_CLOUD_UI.size());
        directive.MutableOpenUriDirective()->SetUri(forecastUri);
        BodyBuilder_.AddDirective(std::move(directive));
        LOG_INFO(Ctx_.Ctx.Logger()) << "Add open_uri directive with cloud_ui";
    }

    if (IsFeedbackInput_) {
        BodyBuilder_.SetShouldListen(false);
    } else {
        const bool isVoiceInput = RunRequest_.BaseRequestProto().GetInterfaces().GetVoiceSession() || !RunRequest_.Input().IsTextInput();
        BodyBuilder_.SetShouldListen(isVoiceInput);
    }
}

TRenderer::TButton TRenderer::CreateOpenUriButton(const TString& buttonType, const TString& buttonUri) {
    TString actionId = TString::Join("button_", buttonType);
    TString buttonCaption = NlgWrapper_.RenderPhrase(NNlgTemplateNames::GET_WEATHER_NOWCAST, TString::Join("render_button_caption__", buttonType), NlgData_).Text;

    TFrameAction action;

    TDirective directive;
    TOpenUriDirective& openUriDirective = *directive.MutableOpenUriDirective();
    openUriDirective.SetUri(buttonUri);
    *action.MutableDirectives()->AddList() = std::move(directive);

    BodyBuilder_.AddAction(actionId, std::move(action));
    // TODO Is it necessary?
    //BodyBuilder_.AddActionSuggest(actionId).Title(buttonCaption);

    TRenderer::TButton button;

    button.SetTitle(buttonCaption);
    button.SetActionId(actionId);
    return button;
}

} // namespace NAlice::NHollywood::NWeather
