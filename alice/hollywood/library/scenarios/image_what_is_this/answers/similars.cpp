#include <alice/hollywood/library/scenarios/image_what_is_this/answers/similars.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/tag.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/ocr.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/context.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/utils.h>
#include <alice/library/client/protos/client_info.pb.h>
#include <alice/library/url_builder/url_builder.h>

#include <util/string/subst.h>

using namespace NAlice::NHollywood::NImage;
using namespace NAlice::NHollywood::NImage::NAnswers;

namespace NAlice::NHollywood::NImage::NFlags {

const TString ENABLE_SNIPPET_INSTEAD_TITLE = "image_recognizer_snippet_instead_title";

}

namespace NAlice::NHollywood::NImage::NComputerVisionFeedbackOptions {
constexpr TStringBuf UNSIMILAR = "feedback_negative_images__unsimilar";
}

namespace {

TString MakeImageUrl(const TStringBuf thumb) {
    TStringBuilder image;
    image << TStringBuf("https:") << thumb;
    if (image.Contains('?')) {
        image << TStringBuf("&");
    } else {
        image << TStringBuf("?");
    }
    image << TStringBuf("n=13");
    return image;
}

constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_similar";
constexpr TStringBuf SHORT_ANSWER_NAME = "similar";
constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_similar";

constexpr TStringBuf ALICE_SMART_MODE = "smartcamera";

}

TSimilars::TSimilars()
    : IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
{
     AllowedIntents = {
         NImages::NCbir::ECbirIntents::CI_CLOTHES,
         NImages::NCbir::ECbirIntents::CI_MARKET,
         NImages::NCbir::ECbirIntents::CI_OCR,
         NImages::NCbir::ECbirIntents::CI_OCR_VOICE
     },
     LastForceAlternativeSuggest = {
         NImages::NCbir::ECbirIntents::CI_SIMILAR_ARTWORK,
         NImages::NCbir::ECbirIntents::CI_SIMILAR_PEOPLE,
         NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE,
         NImages::NCbir::ECbirIntents::CI_INFO
     };
    Intent = NImages::NCbir::ECbirIntents::CI_SIMILAR;
    IntentButtonIcon = "https://avatars.mds.yandex.net/get-images-similar-mturk/13615/icon-what/orig";
    IsSupportSmartCamera = true;

    AliceSmartMode = ALICE_SMART_MODE;
}

bool TSimilars::AddSimilarsGallery(TImageWhatIsThisApplyContext& ctx) const {
    constexpr size_t similarsGallerySizeLimit = 10;

    if (!CheckForSimilarAnswer(ctx)) {
        return false;
    }

    const NSc::TValue& cvData = ctx.GetImageAliceResponse().GetRef();

    NSc::TValue galleryDivCard;
    size_t count = 0;
    for (const auto& item : cvData["Similars"].GetArray()) {
        NSc::TValue galleryItem;
        galleryItem["image"] = MakeImageUrl(item["thmb_href"].GetString());
        galleryItem["link"].SetString(
                NAlice::GenerateSimilarsGalleryLink(ctx.GetClientInfo(), ctx.GetCbirId().GetRef(), item["url"].GetString()));
        if (ctx.HasFlag(NFlags::ENABLE_SNIPPET_INSTEAD_TITLE)) {
            galleryItem["title"] = item["text"].GetString();
        } else {
            galleryItem["title"] = item["title"].GetString();
        }
        galleryItem["html_href"] = item["html_href"].GetString();
        galleryItem["html_host"] = CleanUrl(item["html_host"].GetString());
        galleryDivCard["similar_gallery"].Push(galleryItem);
        count += 1;
        if (count >= similarsGallerySizeLimit) {
            galleryDivCard["tail_link"] = ctx.GenerateImagesSearchUrl("similar", TStringBuf("imageview"),
                                                                      /* disable ptr */ false, "similar");
            break;
        }
    }

    ctx.AddDivCardBlock(TStringBuf("image_similar_gallery"), std::move(galleryDivCard));
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::UNSIMILAR);

    ctx.GetAnalyticsInfoBuilder().AddObject("similar_gallery", "similar_gallery", "1");

    return true;
}

bool TSimilars::CheckForSimilarAnswer(TImageWhatIsThisApplyContext& ctx) const {
    const TMaybe<NSc::TValue>& imageAliceResponseMaybe = ctx.GetImageAliceResponse();
    if (!imageAliceResponseMaybe.Defined()) {
       return false;
    }
    const NSc::TValue& imageAliceResponse = imageAliceResponseMaybe.GetRef();

    return (imageAliceResponse["Similars"].ArraySize() > 0 && ctx.GetCbirId().Defined());
}

TSimilars* TSimilars::GetPtr() {
    static TSimilars* answer = new TSimilars;
    return answer;
}

bool TSimilars::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool /* force */) const {
    return CheckForSimilarAnswer(ctx);
}

void TSimilars::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
    TTag* answerTag = TTag::GetPtr();
    if (answerTag->CheckForTagAnswer(ctx) && answerTag->AddTagAnswer(ctx)) {
        ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::OFFENSIVE_ANSWER);
    } else {
        ctx.StatIncCounter("hollywood_computer_vision_result_answer_is_similar_no_tag");
        ctx.AddTextCard("render_similars", {});
    }

    AddSimilarsGallery(ctx);

    TOcr* answerOcr = TOcr::GetPtr();
    answerOcr->AddOcrAnswer(ctx, EOcrResultCategory::ORC_VAGUE);
}

NSc::TValue TSimilars::GetSwitchSuggestData(TImageWhatIsThisApplyContext& ctx) const {
    NSc::TValue data;
    TMaybe<NSc::TValue> aliceData = ctx.GetImageAliceResponse();
    data["is_face"] = aliceData.Defined() && aliceData.GetRef()["Faces"].ArraySize() > 0;
    return data;
}
