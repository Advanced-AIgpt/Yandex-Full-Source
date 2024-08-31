#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/context.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer_library.h>

#include <util/string/join.h>

using namespace NAlice::NHollywood::NImage;
using namespace NAlice::NHollywood::NImage::NAnswers;

namespace {

const TString FEEDBACK_NAME = "alice.image_what_is_this_feedback";

}

namespace NAlice::NHollywood::NImage::NComputerVisionFeedbackOptions {

const TString POSITIVE = "feedback_positive_images";
const TString NEGATIVE = "feedback_negative_images";

}


IAnswer::IAnswer(TStringBuf answerName, TStringBuf shortAnswerName, TStringBuf disableFlag)
    : AnswerName(answerName)
    , ShortAnswerName(shortAnswerName)
    , DisableFlag(disableFlag)
{
}

TStringBuf IAnswer::GetAnswerName() const {
    return AnswerName;
}

TMaybe<ECaptureMode> IAnswer::GetCaptureMode() const {
    return CaptureMode;
}

bool IAnswer::GetIsFrontalCaptureMode() const {
    return IsFrontalCaptureMode;
}

bool IAnswer::GetIsRepeatable() const {
    return IsRepeatable;
}

TStringBuf IAnswer::GetDisableFlag() const {
    return DisableFlag;
}

TMaybe<NImages::NCbir::ECbirIntents> IAnswer::GetIntent() const {
    return Intent;
}

void IAnswer::MakeRequests(TImageWhatIsThisApplyContext& ctx) const {
    if (!ctx.GetImageAliceResponse().Defined()) {
        ctx.AddImageAliceRequest();
    }
}

void IAnswer::AppendFeedbackOptions(TImageWhatIsThisApplyContext& ctx) const {
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::USELESS);
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::OTHER);
}

TStringBuf IAnswer::GetTrueAnswerName(TImageWhatIsThisApplyContext&) const {
    return GetAnswerName();
}

TStringBuf IAnswer::GetShortAnswerName(TImageWhatIsThisApplyContext&) const {
    return ShortAnswerName;
}

TStringBuf IAnswer::GetAliceMode() const {
    return AliceMode;
}

TMaybe<TStringBuf> IAnswer::GetAliceSmartMode(const TImageWhatIsThisRunContext& ctx) const {
    Y_UNUSED(ctx);
    return AliceSmartMode;
}

TMaybe<TStringBuf> IAnswer::GetAliceSmartMode(const TImageWhatIsThisApplyContext& ctx) const {
    // Only for compilation
    Y_UNUSED(ctx);
    return Nothing();
}

TMaybe<ECaptureMode> IAnswer::GetOpenCaptureMode() const {
    return CaptureMode;
}

void IAnswer::RenderError(TImageWhatIsThisApplyContext& ctx) const {
    ctx.AddSpecialSuggest(NComputerVisionFeedbackOptions::POSITIVE);
    ctx.AddSpecialSuggest(NComputerVisionFeedbackOptions::NEGATIVE);

    if (!RenderErrorAnswer(ctx)) {
        ctx.AddTextCard("render_cannot_recognize_error", {});
    }

    ctx.StatIncCounter("hollywood_computer_vision_cannot_apply_" + TString(GetShortAnswerName(ctx)));

    AddIntentButtonsAndSuggests(ctx);
    ctx.AddOnboardingSuggest();
}

bool IAnswer::RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const {
    if (ctx.GetImageAliceResponse().Defined()) {
        ctx.AddTextCard("render_common_error", {});
        return true;
    }
    return false;
}

void IAnswer::Compose(TImageWhatIsThisApplyContext& ctx) const {
    ctx.AddSpecialSuggest(NComputerVisionFeedbackOptions::POSITIVE);
    ctx.AddSpecialSuggest(NComputerVisionFeedbackOptions::NEGATIVE);

    ComposeAnswer(ctx);

    ctx.StatIncCounter("hollywood_computer_vision_result_answer_is_" + TString(GetShortAnswerName(ctx)));

    AddIntentButtonsAndSuggests(ctx);
    ctx.AddOnboardingSuggest();
}

void IAnswer::CleanUp(TImageWhatIsThisApplyContext& ctx) const {
    ctx.GetState().ClearOfficeLensScan();
}

NSc::TValue IAnswer::GetAnswerSwitchingDescriptor(TImageWhatIsThisApplyContext& ctx) const {
    NSc::TValue descr;
    if (IntentButtonIcon.Defined() && IsSuggestible(ctx)) {
        descr["id"] = GetAnswerName();
        descr["icon"] = IntentButtonIcon.GetRef();
        const NSc::TValue& switchSuggestData = GetSwitchSuggestData(ctx);
        if (!switchSuggestData.IsNull()) {
            descr["data"] = switchSuggestData;
        }
    }
    return descr;
}

NSc::TValue IAnswer::GetSwitchSuggestData(TImageWhatIsThisApplyContext& /*ctx*/) const {
    return NSc::TValue();
}

bool IAnswer::DisabledByFlag(TImageWhatIsThisApplyContext& ctx) const {
    return ctx.HasFlag(ToString(GetDisableFlag()));
}

bool IAnswer::IsSuitable(TImageWhatIsThisApplyContext& ctx, bool force) const {
    return !DisabledByFlag(ctx) && IsSuitableAnswer(ctx, force);
}

bool IAnswer::IsSuggestible(TImageWhatIsThisApplyContext& ctx) const {
    return !DisabledByFlag(ctx) && IsSuggestibleAnswer(ctx);
}

bool IAnswer::GetIsSwitchableTo() const {
    return IsSwitchableTo;
}

bool IAnswer::GetIsSupportSmartCamera() const {
    return IsSupportSmartCamera;
}

bool IAnswer::GetIsForceable() const {
    return IsForceable;
}

bool IAnswer::IsSuggestibleAnswer(TImageWhatIsThisApplyContext& ctx) const {
    return IsSuitableAnswer(ctx, false);
}

void IAnswer::AttachAlternativeIntentsSuggest(TImageWhatIsThisApplyContext& ctx) const {
    NSc::TValue intentsButtonsDivCard;
    auto addIntent = [&intentsButtonsDivCard, &ctx] (NImages::NCbir::ECbirIntents intent) {
        IAnswer* alternativeIntent = TAnswerLibrary::GetAnswerByIntent(intent);
        if (!alternativeIntent) {
            return;
        }
        NSc::TValue descr = alternativeIntent->GetAnswerSwitchingDescriptor(ctx);
        if (descr.IsNull()) {
            return;
        }

        auto& context = descr["context"].SetDict();
        context["cbir_id"].SetString(ctx.GetCbirId().GetRef());
        context["query_url"].SetString(ctx.GetImageUrl());
        ctx.AddSuggest(descr["id"].GetString());
        const TString actionId = ctx.AddSwitchIntentAction(TString(descr["id"].GetString()));

        ctx.AddActionSuggest(TString(descr["id"].GetString()), actionId, descr["data"].ToJsonValue());
        intentsButtonsDivCard.Push(std::move(descr));
    };

    for (const auto& intent : FirstForceAlternativeSuggest) {
        addIntent(intent);
    }
    for (const auto& intent : ctx.GetCbirIntents()) {
        if (AllowedIntents.contains(intent)) {
            addIntent(intent);
        }
    }
    for (const auto& intent : LastForceAlternativeSuggest) {
        addIntent(intent);
    }
    if (!intentsButtonsDivCard.IsNull()) {
        NSc::TValue intentsDivCard;
        intentsDivCard["buttons"] = intentsButtonsDivCard;
        ctx.AddDivCardBlock(TStringBuf("image__alternative_intents"), std::move(intentsDivCard));
    }
}

void IAnswer::AddIntentButtonsAndSuggests(TImageWhatIsThisApplyContext& ctx) const {
    AttachAlternativeIntentsSuggest(ctx);
    AppendFeedbackOptions(ctx);

    ctx.AddSpecialSuggestAction(FEEDBACK_NAME, NComputerVisionFeedbackOptions::POSITIVE);
    const TVector<TStringBuf>& negativeFeedbackOptinos = ctx.GetNegativeFeedbackOptions();
    ctx.AddSpecialSuggestAction(FEEDBACK_NAME, NComputerVisionFeedbackOptions::NEGATIVE,
                                JoinRange(";", negativeFeedbackOptinos.begin(), negativeFeedbackOptinos.end()));
}
