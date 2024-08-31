#include <alice/hollywood/library/scenarios/image_what_is_this/answers/tag.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/ocr.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/similars.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/context.h>

using namespace NAlice::NHollywood::NImage;
using namespace NAlice::NHollywood::NImage::NAnswers;

namespace NAlice::NHollywood::NImage::NFlags {

const TString ENABLE_COMMON_TAGS("image_what_is_this_enable_common_tags");
const TString DISABLE_TAG("image_what_is_this_disable_tag");

}

namespace {

constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_tag";
constexpr TStringBuf SHORT_ANSWER_NAME = "tag";
constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_tag";

}

TTag::TTag()
    : IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
{
    Intent = NImages::NCbir::ECbirIntents::CI_TAGS;
}

bool TTag::HasTagAnswer(TImageWhatIsThisApplyContext& ctx) const {
    const TMaybe<NSc::TValue>& imageAliceResponse = ctx.GetImageAliceResponse();
    if (!imageAliceResponse.Defined()) {
        return false;
    }
    return imageAliceResponse.GetRef()["Tags"].ArraySize() > ctx.GetUsedTagNo();
}

bool TTag::CheckForTagAnswer(TImageWhatIsThisApplyContext& ctx) const {
    const auto thresholdTagMinimum = ctx.HasFlag(NFlags::ENABLE_COMMON_TAGS)
        ? NImages::NCbir::ETagConfidenceCategory::TCC_TINY
        : NImages::NCbir::ETagConfidenceCategory::TCC_LOW;

    return (HasTagAnswer(ctx)
            && !ctx.HasFlag(NFlags::DISABLE_TAG)
            && (GetTagConfidenceCategory(ctx) >= thresholdTagMinimum));
}

NImages::NCbir::ETagConfidenceCategory TTag::GetTagConfidenceCategory(TImageWhatIsThisApplyContext& ctx) const {
    NImages::NCbir::ETagConfidenceCategory tagConfidence;
    return TryFromString(ctx.GetImageAliceResponse().GetRef().TrySelect("/TagConfidence").GetString(), tagConfidence)
        ? tagConfidence
        : NImages::NCbir::ETagConfidenceCategory::TCC_NONE;
}

bool TTag::AddTagAnswer(TImageWhatIsThisApplyContext& ctx, const TString& cardName) const {
    const auto& tag = ctx.GetImageAliceResponse().GetRef().TrySelect(ctx.GetUsedTagPath() + "[0]");
    if (tag.IsNull()) {
        return false;
    }
    const auto& confidence = GetTagConfidenceCategory(ctx);
    if (NImages::NCbir::ETagConfidenceCategory::TCC_NONE == confidence) {
        return false;
    }

    PutTag(tag, confidence, ctx, cardName);

    return true;
}

void TTag::PutTag(const NSc::TValue& tag, NImages::NCbir::ETagConfidenceCategory confidence, TImageWhatIsThisApplyContext& ctx, const TString& cardName) const {
    //static const TStringBuf slotLabel("search_results");

    NSc::TValue textCardData;
    textCardData["tag"] = tag;
    textCardData["tag_confidence"] = ToString(confidence);
    ctx.AddTextCard(cardName, std::move(textCardData));
    ctx.AddSearchSuggest(tag.GetString());
    //AttachUtteranceSuggest(tag.GetString());
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::TAG_WRONG);

    // TODO: Why we fill slot?
    //NSc::TValue searchResults;
    //searchResults["serp"]["url"].SetString(GenerateSearchUri(&HandlerContext(), tag));
    //HandlerContext().CreateSlot(slotLabel, slotLabel, /* optional */ true, std::move(searchResults));

    ctx.GetAnalyticsInfoBuilder().AddObject("tag", "tag", TString(tag.GetString()));
    ctx.StatIncCounter(TStringBuilder() << TStringBuf("hollywood_computer_vision_result_answer_tag_")
                                         << ToString(confidence));

}

TTag* TTag::GetPtr() {
    static TTag* answer = new TTag;
    return answer;
}


bool TTag::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const {
    return HasTagAnswer(ctx) && (force || CheckForTagAnswer(ctx));
}

void TTag::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
    if (AddTagAnswer(ctx)) {
        TSimilars* similarAnswer = TSimilars::GetPtr();
        similarAnswer->AddSimilarsGallery(ctx);

        TOcr* ocrAnswer = TOcr::GetPtr();
        ocrAnswer->AddOcrAnswer(ctx, EOcrResultCategory::ORC_VAGUE);
        //ctx.AttachSimilarSearchSuggest(false /* showButton */);
        ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::OFFENSIVE_ANSWER);
    } else {
        ctx.AddError("missed_tag");
        ctx.StatIncCounter("hollywood_computer_vision_result_error_missed_tag");
    }
}

NSc::TValue TTag::GetSwitchSuggestData(TImageWhatIsThisApplyContext& ctx) const {
    NSc::TValue data;
    TMaybe<NSc::TValue> aliceData = ctx.GetImageAliceResponse();
    data["is_face"] = aliceData.Defined() && aliceData.GetRef()["Faces"].ArraySize() > 0;
    return data;
}
