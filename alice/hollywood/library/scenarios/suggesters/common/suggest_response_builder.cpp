#include "suggest_response_builder.h"

#include "utils.h"

#include <alice/hollywood/library/frame/callback.h>

#include <library/cpp/iterator/concatenate.h>

#include <util/generic/is_in.h>

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf RENDER_CANNOT_RECOMMEND_NLG_PHRASE = "render_cannot_recommend_anymore";
constexpr TStringBuf RENDER_IRRELEVANT_NLG_PHRASE = "render_irrelevant";
constexpr TStringBuf RENDER_NLG_PHRASE = "render_result";

const TString SLOT_DECLINE = "decline";
const TString SLOT_CONFIRM = "confirm";

const TString ACTION_BUTTON = "_button";

const TString DECLINE_BY_HINTS = "_by_hints";
const TString DECLINE_BY_FRAME = "_by_frame";

const TVector<TString> DECLINE_PHRASES = {
    "а другого нет",
    "а еще чего нибудь",
    "блин не надо",
    "вовсе нет",
    "да нет не это",
    "даже не знаю",
    "дальше",
    "дизлайк",
    "другая",
    "другое",
    "другое хотел",
    "другой",
    "другой вариант",
    "еще вариант",
    "еще что нибудь",
    "лучше не надо",
    "мне кажется нет",
    "мне не нравится",
    "может не надо",
    "на следующий",
    "не будем",
    "не буду",
    "не думаю",
    "не люблю",
    "не люблю такое",
    "не надо",
    "не надо лучше",
    "не сильно хочется",
    "не собираюсь",
    "не совсем",
    "не стоит",
    "не уверен",
    "не угадала",
    "не хочется",
    "не хочу",
    "не хочу ничего",
    "не хочу просто",
    "не хочу такое",
    "не этот",
    "неохота",
    "нет",
    "нет другое",
    "нет другой",
    "нет короче",
    "нет не то",
    "нет пожалуй",
    "определенно нет",
    "пожалуй нет",
    "пока не буду",
    "пока не стоит",
    "пока не хочу",
    "прости другое хотел",
    "просто не хочу",
    "следующая",
    "следующее",
    "следующий",
    "совсем не хочу",
    "сомневаюсь",
    "спасибо не буду",
    "хочу другой",
    "что нибудь другое",
    "что то другое",
    "это не то",
    "я этого не хочу",
};

TResponseBodyBuilder& CreateResponseBodyBuilder(const TMaybe<TFrame>& frame, TRunResponseBuilder& builder) {
    if (!frame) {
        return builder.CreateResponseBodyBuilder();
    }
    return builder.CreateResponseBodyBuilder(frame.Get());
}

TMaybe<TFrame> FindSemanticFrame(const TScenarioInputWrapper& input,
                                 const TVector<TString>& acceptedFrameNames)
{
    if (const TMaybe<TFrame> frame = GetCallbackFrame(input.GetCallback())) {
        return *frame;
    }

    for (const auto& frame : input.Proto().GetSemanticFrames()) {
        if (IsIn(acceptedFrameNames, frame.GetName())) {
            return TFrame::FromProto(frame);
        }
    }

    return Nothing();
}

} // namespace

TBaseSuggestResponseBuilder::TBaseSuggestResponseBuilder(TRTLogger& logger,
                                                         const TScenarioRunRequestWrapper& request,
                                                         const TConfig& config,
                                                         TRunResponseBuilder& builder)
    : Logger(logger)
    , Request(request)
    , Config(config)
    , Builder(builder)
    , Frame(FindSemanticFrame(request.Input(), config.AcceptedFrameNames))
    , BodyBuilder(CreateResponseBodyBuilder(Frame, builder))
{
}

TMaybe<TString> TBaseSuggestResponseBuilder::TryGetSlotValue(const TStringBuf slotName) const {
    Y_ENSURE(Frame, "Scenario without semantic frame should be irrelevant");

    if (const auto slot = Frame->FindSlot(slotName)) {
        return slot->Value.AsString();
    }
    return Nothing();
}

void TBaseSuggestResponseBuilder::AddNlg(const TMaybe<TString>& suggestText) {
    TNlgData nlgData(Logger, Request);

    TStringBuf phraseName = RENDER_CANNOT_RECOMMEND_NLG_PHRASE;
    if (suggestText) {
        // TODO(dan-anastasev): pass voice too
        nlgData.Context["suggest_text"] = *suggestText;
        phraseName = RENDER_NLG_PHRASE;
    }

    BodyBuilder.AddRenderedTextWithButtonsAndVoice(Config.NlgTemplate, phraseName, /* buttons = */ {}, nlgData);
}

void TBaseSuggestResponseBuilder::AddConfirmAction(TSemanticFrame&& frameEffect) {
    Y_ENSURE(Frame, "Scenario without semantic frame should be irrelevant");

    NScenarios::TFrameAction onConfirmAction;

    onConfirmAction.MutableNluHint()->SetFrameName(Config.ConfirmGranetName);
    *onConfirmAction.MutableFrame() = std::move(frameEffect);

    BodyBuilder.AddAction(SLOT_CONFIRM, std::move(onConfirmAction));

    if (Config.ConfirmButtonTitle) {
        AddTypeTextSuggest(*Config.ConfirmButtonTitle, SLOT_CONFIRM + ACTION_BUTTON, BodyBuilder);
    }
}

void TBaseSuggestResponseBuilder::AddDeclineAction(const TMaybe<TString>& persuadeAboutItemId) {
    // TODO(dan-anastasev): Make sure that hints work correctly and return match by them
    AddDeclineAction(persuadeAboutItemId, /* matchByHints= */ false);

    if (Config.DeclineButtonTitle) {
        AddTypeTextSuggest(*Config.DeclineButtonTitle, SLOT_DECLINE + ACTION_BUTTON, BodyBuilder);
    }
}

void TBaseSuggestResponseBuilder::AddDeclineAction(const TMaybe<TString>& persuadeAboutItemId, bool matchByHints) {
    Y_ENSURE(Frame, "Scenario without semantic frame should be irrelevant");

    const TString actionName = SLOT_DECLINE + (matchByHints ? DECLINE_BY_HINTS : DECLINE_BY_FRAME);
    NScenarios::TFrameAction onDeclineAction;

    if (matchByHints) {
        *onDeclineAction.MutableNluHint() = BuildFrameNluHint(Concatenate(DECLINE_PHRASES, Config.DeclinePhrases));
    }
    onDeclineAction.MutableNluHint()->SetFrameName(Config.DeclineGranetName);

    auto frame = InitCallbackFrameEffect(Frame->ToProto(), Config.DeclineEffectFrameName,
                                         Config.DeclineEffectFrameCopiedSlots);
    auto& slot = *frame.AddSlots();
    if (!persuadeAboutItemId) {
        slot.SetName(SLOT_DECLINE);
    } else {
        slot.SetName(SLOT_PERSUADE);
        slot.SetType("string");
        slot.SetValue(*persuadeAboutItemId);
    }

    *onDeclineAction.MutableCallback() = ToCallback(frame);

    BodyBuilder.AddAction(actionName, std::move(onDeclineAction));
}

std::unique_ptr<NScenarios::TScenarioRunResponse> TBaseSuggestResponseBuilder::BuildIrrelevantResponse() && {
    Builder.SetIrrelevant();

    TNlgData nlgData{Logger, Request};
    BodyBuilder.AddRenderedText(Config.NlgTemplate, RENDER_IRRELEVANT_NLG_PHRASE, nlgData);

    return std::move(Builder).BuildResponse();
}

} // namespace NAlice::NHollywood
